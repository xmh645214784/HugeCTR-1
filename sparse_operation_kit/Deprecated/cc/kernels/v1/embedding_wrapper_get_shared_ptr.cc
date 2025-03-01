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

namespace HugeCTR {
namespace Version1 {

/** This function is used to get embedding params.
*/
template <typename TypeKey, typename TypeFP>
auto EmbeddingWrapper<TypeKey, TypeFP>::get_embedding_params(const std::string& name) 
     -> std::shared_ptr<EmbeddingParams> {
    auto it = embedding_params_.find(name);
    if (it != embedding_params_.end()) {
        return it->second;
    } else {
        return nullptr;
    }
}

/** This function is used to get registered input spaces. 
* If cannot find spaces for name, return nullptr.
* @param name, get input space for which embedding instance.
*/
template <typename TypeKey, typename TypeFP>
auto EmbeddingWrapper<TypeKey, TypeFP>::get_input_space(const std::string& space_name) 
     -> std::shared_ptr<InputSpace> {
    auto it = input_spaces_.find(space_name);
    if (it != input_spaces_.end()) {
        return it->second;
    } else {
        return nullptr;
    }
}

/** This function is used to get embedding instance.
* @param name, embedding instance name
*/
template <typename TypeKey, typename TypeFP>
std::shared_ptr<IEmbedding> EmbeddingWrapper<TypeKey, TypeFP>::get_embedding(const std::string& embedding_name) {
    auto it = embeddings_.find(embedding_name);
    if (it != embeddings_.end()) {
        return it->second;
    } else {
        return nullptr;
    }
}

/** This function is used to get embedding instance's buff
* @param name, name for this buff. Return empty vector if cannot find this buff.
*/
template <typename TypeKey, typename TypeFP>
std::vector<std::shared_ptr<GeneralBuffer2<CudaAllocator>>> 
EmbeddingWrapper<TypeKey, TypeFP>::get_buff(const std::string& embedding_name) {
    auto it = buffs_.find(embedding_name);
    if (it != buffs_.end()) {
        return it->second;
    } else {
        return std::vector<std::shared_ptr<GeneralBuffer2<CudaAllocator>>>();
    }
}


template <typename TypeKey, typename TypeFP>
template <typename Item>
Item EmbeddingWrapper<TypeKey, TypeFP>::get_item_from_map(
    const std::map<std::string, Item>& map,
    const std::string& map_key) {
    auto it = map.find(map_key);
    if (it != map.end()) {
        return it->second;
    } else {
        return nullptr;
    }
}

template <>
template <>
std::vector<cudaEvent_t> EmbeddingWrapper<long long, float>::get_item_from_map(
    const std::map<std::string, std::vector<cudaEvent_t>>& map,
    const std::string& map_key) {
    auto it = map.find(map_key);
    
    return (it != map.end()) ? it->second : std::vector<cudaEvent_t>();
}
template <>
template <>
std::vector<cudaEvent_t> EmbeddingWrapper<long long, __half>::get_item_from_map(
    const std::map<std::string, std::vector<cudaEvent_t>>& map,
    const std::string& map_key) {
    auto it = map.find(map_key);
    
    return (it != map.end()) ? it->second : std::vector<cudaEvent_t>();
}
template <>
template <>
std::vector<cudaEvent_t> EmbeddingWrapper<unsigned int, float>::get_item_from_map(
    const std::map<std::string, std::vector<cudaEvent_t>>& map,
    const std::string& map_key) {
    auto it = map.find(map_key);
    
    return (it != map.end()) ? it->second : std::vector<cudaEvent_t>();
}
template <>
template <>
std::vector<cudaEvent_t> EmbeddingWrapper<unsigned int, __half>::get_item_from_map(
    const std::map<std::string, std::vector<cudaEvent_t>>& map,
    const std::string& map_key) {
    auto it = map.find(map_key);
    
    return (it != map.end()) ? it->second : std::vector<cudaEvent_t>();
}


template auto EmbeddingWrapper<long long, float>::get_embedding_params(const std::string& name) 
     -> std::shared_ptr<EmbeddingParams>;
template auto EmbeddingWrapper<long long, __half>::get_embedding_params(const std::string& name) 
     -> std::shared_ptr<EmbeddingParams>;
template auto EmbeddingWrapper<unsigned int, float>::get_embedding_params(const std::string& name) 
     -> std::shared_ptr<EmbeddingParams>;  
template auto EmbeddingWrapper<unsigned int, __half>::get_embedding_params(const std::string& name) 
     -> std::shared_ptr<EmbeddingParams>;
template auto EmbeddingWrapper<long long, float>::get_input_space(const std::string& space_name) 
     -> std::shared_ptr<InputSpace>;
template auto EmbeddingWrapper<long long, __half>::get_input_space(const std::string& space_name) 
     -> std::shared_ptr<InputSpace>;
template auto EmbeddingWrapper<unsigned int, float>::get_input_space(const std::string& space_name) 
     -> std::shared_ptr<InputSpace>;
template auto EmbeddingWrapper<unsigned int, __half>::get_input_space(const std::string& space_name) 
     -> std::shared_ptr<InputSpace>;
template std::shared_ptr<IEmbedding> EmbeddingWrapper<long long, float>::get_embedding(
                                    const std::string& embedding_name);
template std::shared_ptr<IEmbedding> EmbeddingWrapper<long long, __half>::get_embedding(
                                    const std::string& embedding_name);
template std::shared_ptr<IEmbedding> EmbeddingWrapper<unsigned int, float>::get_embedding(
                                    const std::string& embedding_name);
template std::shared_ptr<IEmbedding> EmbeddingWrapper<unsigned int, __half>::get_embedding(
                                    const std::string& embedding_name);
template std::vector<std::shared_ptr<GeneralBuffer2<CudaAllocator>>> 
EmbeddingWrapper<long long, float>::get_buff(const std::string& embedding_name);
template std::vector<std::shared_ptr<GeneralBuffer2<CudaAllocator>>> 
EmbeddingWrapper<long long, __half>::get_buff(const std::string& embedding_name);
template std::vector<std::shared_ptr<GeneralBuffer2<CudaAllocator>>> 
EmbeddingWrapper<unsigned int, float>::get_buff(const std::string& embedding_name);
template std::vector<std::shared_ptr<GeneralBuffer2<CudaAllocator>>> 
EmbeddingWrapper<unsigned int, __half>::get_buff(const std::string& embedding_name);


template auto EmbeddingWrapper<unsigned int, float>::get_item_from_map(
    const std::map<std::string, std::shared_ptr<EmbeddingParams>>& map, const std::string& map_key)
    -> std::shared_ptr<EmbeddingParams>;
template auto EmbeddingWrapper<long long, float>::get_item_from_map(
    const std::map<std::string, std::shared_ptr<EmbeddingParams>>& map, const std::string& map_key)
    -> std::shared_ptr<EmbeddingParams>;
template auto EmbeddingWrapper<unsigned int, __half>::get_item_from_map(
    const std::map<std::string, std::shared_ptr<EmbeddingParams>>& map, const std::string& map_key)
    -> std::shared_ptr<EmbeddingParams>;
template auto EmbeddingWrapper<long long, __half>::get_item_from_map(
    const std::map<std::string, std::shared_ptr<EmbeddingParams>>& map, const std::string& map_key)
    -> std::shared_ptr<EmbeddingParams>;

template auto EmbeddingWrapper<unsigned int, float>::get_item_from_map(
    const std::map<std::string, std::shared_ptr<DistributeKeysInternelSpaces>>& map, const std::string& map_key)
    -> std::shared_ptr<DistributeKeysInternelSpaces>;
template auto EmbeddingWrapper<long long, float>::get_item_from_map(
    const std::map<std::string, std::shared_ptr<DistributeKeysInternelSpaces>>& map, const std::string& map_key)
    -> std::shared_ptr<DistributeKeysInternelSpaces>;
template auto EmbeddingWrapper<unsigned int, __half>::get_item_from_map(
    const std::map<std::string, std::shared_ptr<DistributeKeysInternelSpaces>>& map, const std::string& map_key)
    -> std::shared_ptr<DistributeKeysInternelSpaces>;
template auto EmbeddingWrapper<long long, __half>::get_item_from_map(
    const std::map<std::string, std::shared_ptr<DistributeKeysInternelSpaces>>& map, const std::string& map_key)
    -> std::shared_ptr<DistributeKeysInternelSpaces>;

template auto EmbeddingWrapper<unsigned int, float>::get_item_from_map(
    const std::map<std::string, distribute_keys_gpu_func_type>& map, const std::string& map_key)
    -> distribute_keys_gpu_func_type;
template auto EmbeddingWrapper<long long, float>::get_item_from_map(
    const std::map<std::string, distribute_keys_gpu_func_type>& map, const std::string& map_key)
    -> distribute_keys_gpu_func_type;
template auto EmbeddingWrapper<unsigned int, __half>::get_item_from_map(
    const std::map<std::string, distribute_keys_gpu_func_type>& map, const std::string& map_key)
    -> distribute_keys_gpu_func_type;
template auto EmbeddingWrapper<long long, __half>::get_item_from_map(
    const std::map<std::string, distribute_keys_gpu_func_type>& map, const std::string& map_key)
    -> distribute_keys_gpu_func_type;

} // namespace Version1
} // namespace HugeCTR