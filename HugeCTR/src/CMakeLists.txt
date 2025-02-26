# 
# Copyright (c) 2021, NVIDIA CORPORATION.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#      http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

cmake_minimum_required(VERSION 3.8)
include_directories(/opt/conda/include)

include(FetchContent)

FetchContent_Declare(
  pybind11_sources
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG        v2.2
)
FetchContent_GetProperties(pybind11_sources)

if(NOT pybind11_sources_POPULATED)
  FetchContent_Populate(pybind11_sources)
  add_subdirectory(
    ${pybind11_sources_SOURCE_DIR}
    ${pybind11_sources_BINARY_DIR}
    )
endif()

file(GLOB huge_ctr_src
  cpu_resource.cpp
  gpu_resource.cpp
  resource_manager.cpp
  resource_managers/resource_manager_core.cpp
  resource_managers/resource_manager_ext.cpp
  data_simulator.cu
  data_reader.cpp
  layer.cpp
  layers/*.cu
  loss.cu
  network.cu
  network.cpp
  inference/hugectrmodel.cpp
  inference/session_inference.cpp
  inference/embedding_interface.cpp
  inference/embedding_cache.cpp
  inference/inference_utilis.cpp
  inference/parameter_server.cpp
  inference/unique_op/unique_op.cu
  inference/embedding_feature_combiner.cu
  inference/embedding_cache.cu
  data_readers/metadata.cpp
  metrics.cu
  optimizers/*.cu
  optimizer.cpp
  regularizer.cu
  regularizers/l1_regularizer.cu
  regularizers/l2_regularizer.cu
  regularizers/no_regularizer.cu
  parser.cpp
  parsers/solver_parser.cpp
  parsers/learning_rate_scheduler_parser.cpp
  parsers/create_optimizer.cpp
  parsers/create_network.cpp
  parsers/create_embedding.cpp
  parsers/create_datareader.cpp
  parsers/inference_parser.cpp
  data_readers/data_collector.cu
  data_readers/parquet_data_converter.cu
  hashtable/nv_hashtable.cu
  embeddings/*.cu
  model_oversubscriber/model_oversubscriber_impl.cpp
  model_oversubscriber/parameter_server.cpp
  model_oversubscriber/parameter_server_manager.cpp
  model_oversubscriber/sparse_model_file.cpp
  model_oversubscriber/sparse_model_entity.cpp
  diagnose.cu
  utils.cu
  ../pybind/model.cpp
  ../pybind/add_input.cpp
  ../pybind/add_sparse_embedding.cpp
  ../pybind/add_dense_layer.cpp
)

set(CMAKE_CXX_STANDARD 17)
add_library(huge_ctr_static STATIC ${huge_ctr_src})
add_library(huge_ctr_shared SHARED ${huge_ctr_src})

target_link_libraries(huge_ctr_static PUBLIC cublas curand cudnn nccl nvToolsExt ${CMAKE_THREAD_LIBS_INIT} cudf stdc++fs tbb)
target_link_libraries(huge_ctr_shared PUBLIC cublas curand cudnn nccl nvToolsExt ${CMAKE_THREAD_LIBS_INIT} cudf stdc++fs tbb)

if(MPI_FOUND)
  target_link_libraries(huge_ctr_static PUBLIC cublas curand cudnn nccl nvToolsExt ${CMAKE_THREAD_LIBS_INIT} ${MPI_CXX_LIBRARIES} hwloc ucp ucs ucm uct cudf stdc++fs numa)
  target_link_libraries(huge_ctr_shared PUBLIC cublas curand cudnn nccl nvToolsExt ${CMAKE_THREAD_LIBS_INIT} ${MPI_CXX_LIBRARIES} hwloc ucp ucs ucm uct cudf stdc++fs numa)
  message(STATUS "${MPI_CXX_LIBRARIES}")
else()
  target_link_libraries(huge_ctr_static PUBLIC cublas curand cudnn nccl nvToolsExt ${CMAKE_THREAD_LIBS_INIT} cudf stdc++fs numa)
  target_link_libraries(huge_ctr_shared PUBLIC cublas curand cudnn nccl nvToolsExt ${CMAKE_THREAD_LIBS_INIT} cudf stdc++fs numa)
endif()

target_link_libraries(huge_ctr_static PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(huge_ctr_static PUBLIC gpu_cache)
target_compile_features(huge_ctr_static PUBLIC cxx_std_17)
set_target_properties(huge_ctr_static PROPERTIES CUDA_RESOLVE_DEVICE_SYMBOLS ON)
set_target_properties(huge_ctr_static PROPERTIES CUDA_ARCHITECTURES OFF)

target_link_libraries(huge_ctr_shared PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(huge_ctr_shared PUBLIC gpu_cache)
target_compile_features(huge_ctr_shared PUBLIC cxx_std_17)
set_target_properties(huge_ctr_shared PROPERTIES CUDA_RESOLVE_DEVICE_SYMBOLS ON)
set_target_properties(huge_ctr_shared PROPERTIES CUDA_ARCHITECTURES OFF)

add_library(hugectr MODULE ../pybind/module_main.cpp)
target_link_libraries(hugectr PUBLIC pybind11::module ${CUDA_LIBRARIES} huge_ctr_shared)
set_target_properties(hugectr PROPERTIES PREFIX "")
