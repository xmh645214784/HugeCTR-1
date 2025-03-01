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

#pragma once
#include <cublas_v2.h>
#include <cudnn.h>
#include <curand.h>
#include <nccl.h>
#include "utils.hpp"

namespace HugeCTR {

/**
 * @brief GPU resource allocated on a target gpu.
 *
 * This class implement unified resource managment on the target GPU.
 */
class GPUResource {
  const int device_id_;
  const size_t global_id_;
  cudaStream_t computation_stream_; /**< cuda stream for computation */
  cudaStream_t memcpy_stream_;      /**< cuda stream for data copy */
  curandGenerator_t replica_uniform_curand_generator_;
  curandGenerator_t replica_variant_curand_generator_;
  cublasHandle_t cublas_handle_;
  cudnnHandle_t cudnn_handle_;
  ncclComm_t comm_;
  size_t sm_count_;
  int cc_major_;
  int cc_minor_;

 public:
  GPUResource(int device_id, size_t global_id, unsigned long long replica_uniform_seed,
              unsigned int long long replica_variant_seed, const ncclComm_t& comm);
  GPUResource(const GPUResource&) = delete;
  GPUResource& operator=(const GPUResource&) = delete;
  ~GPUResource();

  int get_device_id() const { return device_id_; }
  size_t get_global_id() const { return global_id_; }
  const cudaStream_t& get_stream() const { return computation_stream_; }
  const cudaStream_t& get_memcpy_stream() const { return memcpy_stream_; }
  const curandGenerator_t& get_replica_uniform_curand_generator() const {
    return replica_uniform_curand_generator_;
  }
  const curandGenerator_t& get_replica_variant_curand_generator() const {
    return replica_variant_curand_generator_;
  }
  const cublasHandle_t& get_cublas_handle() const { return cublas_handle_; }
  const cudnnHandle_t& get_cudnn_handle() const { return cudnn_handle_; }
  const ncclComm_t& get_nccl() const { return comm_; }
  size_t get_sm_count() const { return sm_count_; }
  int get_cc_major() const { return cc_major_; }
  int get_cc_minor() const { return cc_minor_; }
};

}  // namespace HugeCTR
