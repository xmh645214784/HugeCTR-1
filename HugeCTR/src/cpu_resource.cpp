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

#include <common.hpp>
#include <cpu_resource.hpp>
#include <utils.hpp>

namespace HugeCTR {
CPUResource::CPUResource(unsigned long long replica_uniform_seed,
                         const std::vector<unsigned long long> replica_variant_seeds) {
  replica_uniform_curand_generators_.resize(replica_variant_seeds.size());
  replica_variant_curand_generators_.resize(replica_variant_seeds.size());

  for (size_t i = 0; i < replica_variant_seeds.size(); i++) {
    CK_CURAND_THROW_(curandCreateGeneratorHost(&replica_uniform_curand_generators_[i],
                                               CURAND_RNG_PSEUDO_DEFAULT));
    CK_CURAND_THROW_(curandSetPseudoRandomGeneratorSeed(replica_uniform_curand_generators_[i],
                                                        replica_uniform_seed));
    CK_CURAND_THROW_(curandCreateGeneratorHost(&replica_variant_curand_generators_[i],
                                               CURAND_RNG_PSEUDO_DEFAULT));
    CK_CURAND_THROW_(curandSetPseudoRandomGeneratorSeed(replica_variant_curand_generators_[i],
                                                        replica_variant_seeds[i]));
  }
}

CPUResource::~CPUResource() {
  try {
    for (auto generator : replica_uniform_curand_generators_) {
      CK_CURAND_THROW_(curandDestroyGenerator(generator));
    }
    for (auto generator : replica_variant_curand_generators_) {
      CK_CURAND_THROW_(curandDestroyGenerator(generator));
    }
  } catch (const std::runtime_error& rt_err) {
    std::cerr << rt_err.what() << std::endl;
  }
}
}  // namespace HugeCTR