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

#ifndef COMMON_H
#define COMMON_H

#include <stdexcept>
#include <string>
#include <nccl.h>
#include <ctime>
#include <iomanip>
#include <unordered_map>
#include <regex>
#include <sstream>
#include <iostream>
#include <cstdint>

namespace SparseOperationKit {

#define ErrorBase (std::string(__FILE__) + ":" + std::to_string(__LINE__) + " ")

#define CK_NCCL(cmd)                                                     \
    do {                                                                 \
        ncclResult_t r = (cmd);                                          \
        if (r != ncclSuccess) {                                          \
            throw std::runtime_error(ErrorBase +                         \
                                    std::string(ncclGetErrorString(r))); \
        }                                                                \
    } while (0)

#define CK_CUDA(cmd)                                                        \
    do {                                                                    \
        cudaError_t r = (cmd);                                              \
        if (r != cudaSuccess) {                                             \
            throw std::runtime_error(ErrorBase +                            \
                                     std::string(cudaGetErrorString(r)));   \
        }                                                                   \
    } while (0)

#define CK_CURAND(cmd)                                   \
    do {                                                 \
        curandStatus_t r = (cmd);                        \
        if (r != CURAND_STATUS_SUCCESS) {                \
            throw std::runtime_error(ErrorBase +         \
                                     std::to_string(r)); \
        }                                                \
    } while (0)

#define CK_CUSPARSE(cmd)                                                            \
    do {                                                                            \
        cusparseStatus_t error = (cmd);                                             \
        if (error != CUSPARSE_STATUS_SUCCESS) {                                     \
            throw std::runtime_error(ErrorBase +                                    \
                                     std::string(cusparseGetErrorString(error)));   \
        }                                                                           \
    } while (0)

#define MESSAGE(msg)                                                                                                                \
    do {                                                                                                                            \
        std::cout.setf(std::ios::right, std::ios::adjustfield);                                                                     \
        std::time_t time_instance = std::time(0);                                                                                   \
        std::tm* time_now = std::localtime(&time_instance);                                                                         \
        std::cout << time_now->tm_year + 1900 << "-"                                                                                \
                  << std::setfill('0') << std::setw(2) << std::to_string(1 + time_now->tm_mon)                                      \
                  << "-" << std::setfill('0') << std::setw(2) << std::to_string(time_now->tm_mday) << " "                           \
                  << std::setfill('0') << std::setw(2) << std::to_string(time_now->tm_hour) << ":"                                  \
                  << std::setfill('0') << std::setw(2) << std::to_string(time_now->tm_min)                                          \
                  << ":" << std::resetiosflags(std::ios::fixed) << std::setprecision(4) << std::to_string(time_now->tm_sec) << ":"  \
                  << " I " << __FILE__ << ":" << __LINE__ << "] " << (msg) << std::endl;                                            \
    } while (0)

void ncclUniqueId_to_string(const ncclUniqueId& uniqueId, std::string& uniqueId_s);
void string_to_ncclUniqueId(const std::string& uniqueId_s, ncclUniqueId& uniqueId);
void ncclUniqueId_to_int(const ncclUniqueId& uniqueId, int* uniqueId_num);
void int_to_ncclUniqueId(const int32_t* uniqueId_num, ncclUniqueId& uniqueId);

enum class SparseEmbeddingType {Distributed, Localized};
enum class DenseEmbeddingType {Custom}; // TODO: give it a better name.
enum class CombinerType {Sum, Mean};
enum class OptimizerType {Adam};

extern const std::unordered_map<std::string, SparseEmbeddingType> SparseEmbeddingTypeMap; 
extern const std::unordered_map<std::string, DenseEmbeddingType> DenseEmbeddingTypeMap;
extern const std::unordered_map<std::string, CombinerType> CombinerMap;
extern const std::unordered_map<std::string, OptimizerType> OptimizerMap;
using optimizer_hyper_params = std::unordered_map<std::string, float>;

class OptimizerHyperParamsHandler {
public:
    static std::shared_ptr<OptimizerHyperParamsHandler> create(optimizer_hyper_params&& hyper_params);

    float get_hyper_param(const std::string param_name) const;
private:
    std::unordered_map<std::string, float> hyper_params_;
    explicit OptimizerHyperParamsHandler(optimizer_hyper_params&& hyper_params);
};
using OptimizerHyperParamsHandler_t = std::shared_ptr<OptimizerHyperParamsHandler>;

template <typename type>
void find_item_in_map(const std::unordered_map<std::string, type>& map, const std::string& key, type& value) {
    auto it = map.find(key);
    if (it == map.end()) throw std::runtime_error(ErrorBase + "Count not find " + key + " in map.");
    value = it->second;
}


std::vector<std::string> str_split(const std::string& input_s, const std::string& pattern);
std::string strs_concat(const std::vector<std::string>& str_vector, const std::string& connection);
int32_t string2num(const std::string& str);


bool file_exist(const std::string filename);
void delete_file(const std::string filename);

} // namespace SparseOperationKit


namespace HugeCTR {

class CudaDeviceContext {
    int32_t original_device_;

public:
    CudaDeviceContext();
    CudaDeviceContext(int32_t device); 
    ~CudaDeviceContext() noexcept(false);
    void set_device(int32_t device) const; 
};

} // namespace HugeCTR

#endif // COMMON_H