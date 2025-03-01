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


/** Constructor. 
* @param vvgpu, visibable GPUs. stored in vector<vector>
* @param seed, random seed, default should be 0.  
* @param batch_size, batch size in training process.
* @param batch_size_eval, batch size in evaluation process.
*/
template <typename TypeKey, typename TypeFP>
EmbeddingWrapper<TypeKey, TypeFP>::EmbeddingWrapper(const std::vector<std::vector<int>>& vvgpu, 
                                                    unsigned long long seed,
                                                    long long batch_size, long long batch_size_eval)
    : resource_manager_(HugeCTR::ResourceManager::create(vvgpu, seed)),
    batch_size_(batch_size), batch_size_eval_(batch_size_eval) {
    /*set nccl data type*/
    switch(sizeof(TypeKey)) {
        case 4: { // unsigned int
            nccl_type_ = ncclUint32;
            break;
        }
        case 8: { // long long
            nccl_type_ = ncclInt64;
            break;
        }
    } // switch

}


/*destructor*/
template <typename TypeKey, typename TypeFP>
EmbeddingWrapper<TypeKey, TypeFP>::~EmbeddingWrapper(){
    for (auto& fprop_events_it : fprop_events_) {
        for (auto& event : fprop_events_it.second) {
            cudaEventDestroy(event);
        }
    }
    for (auto& bprop_events_it : bprop_events_) {
        for (auto& event : bprop_events_it.second) {
            cudaEventDestroy(event);
        }
    }
}



template EmbeddingWrapper<long long, float>::EmbeddingWrapper(const std::vector<std::vector<int>>&,
            unsigned long long, long long, long long);
template EmbeddingWrapper<unsigned int, float>::EmbeddingWrapper(const std::vector<std::vector<int>>&,
            unsigned long long, long long, long long);
template EmbeddingWrapper<long long, __half>::EmbeddingWrapper(const std::vector<std::vector<int>>&,
            unsigned long long, long long, long long);
template EmbeddingWrapper<unsigned int, __half>::EmbeddingWrapper(const std::vector<std::vector<int>>&, 
            unsigned long long, long long, long long);

template EmbeddingWrapper<long long, float>::~EmbeddingWrapper();
template EmbeddingWrapper<long long, __half>::~EmbeddingWrapper();
template EmbeddingWrapper<unsigned int, float>::~EmbeddingWrapper();
template EmbeddingWrapper<unsigned int, __half>::~EmbeddingWrapper();

} //namespace Version1
} //namespace HugeCTR