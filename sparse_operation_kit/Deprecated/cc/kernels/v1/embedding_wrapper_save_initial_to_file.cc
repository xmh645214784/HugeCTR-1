/*
* Copyright (c) 2021, NVIDIA CORPORATION.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/


#include "embedding_wrapper.h"
#include "HugeCTR/include/embeddings/distributed_slot_sparse_embedding_hash.hpp"
#include "HugeCTR/include/embeddings/localized_slot_sparse_embedding_hash.hpp"
#include "HugeCTR/include/embeddings/localized_slot_sparse_embedding_one_hot.hpp"
#include "embedding_utils.hpp"

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

namespace HugeCTR {
namespace Version1 {


/** This function is used to store initial values to file.
* @param init_value, shape = [vocabulary, embedding_vec_size].
*/
template <typename TypeKey, typename TypeFP>
tensorflow::Status EmbeddingWrapper<TypeKey, TypeFP>::save_initial_to_file(const std::string& embedding_name, 
                            const tensorflow::Tensor* const init_value, const std::string& save_name,
                            const bool on_gpu) {
    std::shared_ptr<EmbeddingParams> params = get_embedding_params(embedding_name);
    if (!params) return tensorflow::errors::NotFound(__FILE__, ": ", __LINE__, " Not found embedding params for ", embedding_name);

    const std::string key_file = save_name + "/key";
    const std::string slot_file = save_name  + "/slot_id";
    const std::string vec_file = save_name + "/emb_vector";

    if (!fs::exists(save_name)) {
      fs::create_directory(save_name);
    }

    std::ofstream key_stream(key_file, std::ofstream::binary | std::ofstream::trunc);
    if (!key_stream.is_open()) return tensorflow::errors::Unavailable(__FILE__, ": ", __LINE__, " key file is not open.");

    std::ofstream vec_stream(vec_file, std::ofstream::binary | std::ofstream::trunc);
    if (!vec_stream.is_open()) return tensorflow::errors::Unavailable(__FILE__, ": ", __LINE__, " vec file is not open.");

    std::ofstream slot_stream;
    if (params->embedding_type_ != Embedding_t::DistributedSlotSparseEmbeddingHash) {
      slot_stream.open(slot_file, std::ofstream::binary | std::ofstream::trunc);
      if (!slot_stream.is_open()) return tensorflow::errors::Unavailable(__FILE__, ": ", __LINE__, " slot file is not open.");
    }

    if (init_value->dims() != 2) return tensorflow::errors::Unavailable(__FILE__, ": ", __LINE__, " init_value's dims should be 2.");
    if (static_cast<size_t>(init_value->dim_size(0)) > 
        (params->max_vocabulary_size_per_gpu_ * resource_manager_->get_global_gpu_count()))
        return tensorflow::errors::Unavailable(__FILE__, ": ", __LINE__, 
                    " init_value's vocabulary is larger than total vocabulary size.");
    if (init_value->dim_size(1) != params->embedding_vec_size_) return tensorflow::errors::Unavailable(__FILE__, ": ", __LINE__, 
                    " init_value's embedding_vec_size_ is not equal to which is initialized.");

    auto init_value_flat = init_value->flat<float>();
    for (long int row = 0; row < init_value->dim_size(0); ++row) { // each row
        long long key = static_cast<long long>(row); // key
        key_stream.write(reinterpret_cast<char*>(&key), sizeof(long long));

        switch (params->embedding_type_) {
            case Embedding_t::DistributedSlotSparseEmbeddingHash: {
                // embedding vector values
                if (on_gpu) {
                    std::unique_ptr<float []> temp_init_value(new float[params->embedding_vec_size_]());
                    WRAPPER_CUDA_CHECK(cudaMemcpy(temp_init_value.get(), 
                                                   init_value_flat.data() + row * params->embedding_vec_size_,
                                                   sizeof(float) * params->embedding_vec_size_,
                                                   cudaMemcpyDeviceToHost));
                    vec_stream.write(reinterpret_cast<char*>(temp_init_value.get()),
                                     sizeof(float) * params->embedding_vec_size_);
                } else { // on cpu
                    vec_stream.write(reinterpret_cast<const char*>(init_value_flat.data() + row * params->embedding_vec_size_),
                                     sizeof(float) * params->embedding_vec_size_);
                }
                break;
            }
            case Embedding_t::LocalizedSlotSparseEmbeddingOneHot:
            case Embedding_t::LocalizedSlotSparseEmbeddingHash: {
                size_t slot_id = static_cast<size_t>(key) % params->slot_num_; // slot_id
                slot_stream.write(reinterpret_cast<char*>(&slot_id), sizeof(size_t));

                // embedding vector values
                if (on_gpu) {
                    std::unique_ptr<float []> temp_init_value(new float[params->embedding_vec_size_]());
                    WRAPPER_CUDA_CHECK(cudaMemcpy(temp_init_value.get(), 
                                                   init_value_flat.data() + row * params->embedding_vec_size_,
                                                   sizeof(float) * params->embedding_vec_size_,
                                                   cudaMemcpyDeviceToHost));
                    vec_stream.write(reinterpret_cast<char*>(temp_init_value.get()),
                                           sizeof(float) * params->embedding_vec_size_);
                } else { // on cpu
                    vec_stream.write(reinterpret_cast<const char*>(init_value_flat.data() + row * params->embedding_vec_size_),
                                       sizeof(float) * params->embedding_vec_size_);
                }
                break;
            }
            default: {
                return tensorflow::errors::Unavailable(__FILE__, ": ", __LINE__, " Do not support such embedding type.");
            }
        } // switch (params->embedding_type_)
    } // for row

    return tensorflow::Status::OK();
}

template tensorflow::Status EmbeddingWrapper<long long, float>::save_initial_to_file(
                            const std::string& embedding_name, 
                            const tensorflow::Tensor* const init_value, const std::string& save_name,
                            const bool on_gpu);
template tensorflow::Status EmbeddingWrapper<long long, __half>::save_initial_to_file(
                            const std::string& embedding_name, 
                            const tensorflow::Tensor* const init_value, const std::string& save_name,
                            const bool on_gpu);
template tensorflow::Status EmbeddingWrapper<unsigned int, float>::save_initial_to_file(
                            const std::string& embedding_name, 
                            const tensorflow::Tensor* const init_value, const std::string& save_name,
                            const bool on_gpu);
template tensorflow::Status EmbeddingWrapper<unsigned int, __half>::save_initial_to_file(
                            const std::string& embedding_name, 
                            const tensorflow::Tensor* const init_value, const std::string& save_name,
                            const bool on_gpu);



} // namespace Version1
} // namespace HugeCTR