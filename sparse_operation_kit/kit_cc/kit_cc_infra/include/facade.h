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

#ifndef EMBEDDING_PLUGIN_FACADE_H
#define EMBEDDING_PLUGIN_FACADE_H

#include "resources/manager.h"
#include "parameters/manager_interface.h"
#include "embeddings/manager.h"
#include "embedding_variable.h"
#include "optimizer/optimizer_interface.h"
#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/resource_var.h"

#include <string>
#include <memory>

namespace SparseOperationKit {

class Facade final {
private:
    Facade();
    ~Facade() = default;
    Facade(const Facade&) = delete;
    Facade& operator=(const Facade&) = delete;
    Facade(Facade&&) = delete;
    Facade& operator=(Facade&&) = delete;

    std::shared_ptr<ResourcesManager> resources_mgr_;
    std::shared_ptr<ParamsManager> params_mgr_;
    std::shared_ptr<EmbeddingManager> embedding_mgr_;
    std::shared_ptr<Optimizer> optimizer_;

    void try_allocate_memory(const size_t global_replica_id) const;
    void try_allocate_memory() const;

public: 
    static Facade* instance();
    void operator delete(void*);

    void get_nccl_unique_id(std::string& nccl_unique_id);
    void get_nccl_unique_id(int32_t* nccl_unique_id);
    void get_random_seed(uint64_t* seed);

    void init(const size_t global_replica_id, const size_t num_replicas_in_sync, 
              const int32_t* nccl_unique_id, const uint64_t global_seed,
              const size_t global_batch_size, const cudaStream_t& tf_stream);

    void create_variables(const size_t local_replica_id, const float* initial_value, const bool use_hashtable, 
                          const std::vector<int64_t> shape, const std::string name,
                          const bool trainable,
                          tensorflow::core::RefCountPtr<tensorflow::EmbeddingVariable>& emb_variable,
                          tensorflow::Tensor* emb_tensor);
    void create_variables(const size_t local_replica_id, const std::string& initializer, const bool use_hashtable,
                          const std::vector<int64_t> shape, const std::string name,
                          const bool trainable,
                          tensorflow::core::RefCountPtr<tensorflow::EmbeddingVariable>& emb_variable,
                          tensorflow::Tensor* emb_tensor);
    void create_variables(const size_t local_replica_id, float* variable, const bool use_hashtable,
                          const std::vector<int64_t> shape, const std::string name,
                          const bool trainable,
                          tensorflow::core::RefCountPtr<tensorflow::EmbeddingVariable>& emb_variable,
                          tensorflow::Tensor* emb_tensor);
    
    void create_embedding_sparse(const tensorflow::core::RefCountPtr<tensorflow::EmbeddingVariable>& variable,
                                 const std::string input_dispatcher,
                                 const std::vector<std::string>& input_dispatcher_subsequent_ops,
                                 const std::string embedding_executor,
                                 const std::string output_dispatcher,
                                 const std::vector<std::string>& output_dispatcher_subsequent_ops,
                                 const size_t slot_num, const size_t max_nnz, const size_t max_feature_num, 
                                 const std::string combiner, 
                                 tensorflow::Tensor* emb_handle);
    
    void create_embedding_dense(const tensorflow::core::RefCountPtr<tensorflow::EmbeddingVariable>& variable,
                                const std::string input_dispatcher,
                                const std::vector<std::string>& input_dispatcher_subsequent_ops,
                                const std::string embedding_executor,
                                const std::string output_dispatcher,
                                const std::vector<std::string>& output_dispatcher_subsequent_ops,
                                const size_t slot_num, const size_t nnz_per_slot,
                                tensorflow::Tensor* emb_handle);
                          
    void create_optimizer(const std::string optimizer_type,
                          tensorflow::Tensor* optimizer_handle,
                          optimizer_hyper_params hyper_params);

    void get_output_shape(const tensorflow::Tensor* emb_handle,
                          tensorflow::TensorShape& tensor_shape);
    void get_grad_shape(const size_t global_replica_id,
                        const tensorflow::Tensor* emb_handle,
                        tensorflow::TensorShape& grad_shape);

    void forward(const tensorflow::Tensor* emb_handle, 
                 const tensorflow::Tensor* values_tensor,
                 const tensorflow::Tensor* indices_tensor,
                 const size_t global_replica_id,
                 const bool training,
                 tensorflow::Tensor* emb_vector_tensor);
    void forward(const tensorflow::Tensor* emb_handle,
                 const tensorflow::Tensor* values_tensor,
                 const size_t global_replica_id,
                 const bool training,
                 tensorflow::Tensor* emb_vector_tensor);
    void backward(const tensorflow::Tensor* emb_handle,
                  const size_t global_replica_id,
                  const tensorflow::Tensor* top_gradient_tensor,
                  tensorflow::Tensor* gradient_tensor,
                  tensorflow::Tensor* value_index_tensor);
    void apply_gradients(const tensorflow::core::RefCountPtr<tensorflow::EmbeddingVariable>& variable,
                         const tensorflow::Tensor* gradient_tensor,
                         const tensorflow::Tensor* local_indices_tensor,
                         const size_t local_replica_id,
                         const float learning_rate,
                         const size_t current_step);

    void dump_to_file(const tensorflow::core::RefCountPtr<tensorflow::EmbeddingVariable>& emb_variable,
                      const std::string filepath);
    void restore_from_file(tensorflow::core::RefCountPtr<tensorflow::EmbeddingVariable>& emb_variable,
                           const std::string filepath);
    void load_embedding_values(tensorflow::core::RefCountPtr<tensorflow::EmbeddingVariable>& emb_variable,
                             const tensorflow::OpInputList* tensor_list);


    // backdoors for unit test
    const std::shared_ptr<ResourcesManager>& get_resource_mgr() const;
};

// helper function
int GetLocalReplicaIdFromDeviceName(const std::string device_name);

} // namespace SparseOperationKit

#endif