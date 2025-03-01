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

#include "HugeCTR/include/utils.hpp"
#include "HugeCTR/include/data_generator.hpp"
#include "HugeCTR/include/data_readers/data_reader.hpp"
#include "HugeCTR/include/embeddings/localized_slot_sparse_embedding_hash.hpp"
#include "HugeCTR/include/embeddings/distributed_slot_sparse_embedding_hash.hpp"
#include "utest/embedding/embedding_test_utils.hpp"
#include "gtest/gtest.h"
#include "utest/test_utils.h"
#include "utest/test_utils.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

namespace HugeCTR {

namespace mos_test {

template <typename TypeKey>
void load_key_to_vec(std::vector<TypeKey> &key_vec, std::ifstream &key_ifs,
    size_t num_key, size_t key_file_size_in_byte) {
  key_vec.resize(num_key);
  if (std::is_same<TypeKey, long long>::value) {
    key_ifs.read(reinterpret_cast<char *>(key_vec.data()), key_file_size_in_byte);
  } else {
    std::vector<long long> i64_key_vec(num_key, 0);
    key_ifs.read(reinterpret_cast<char *>(i64_key_vec.data()), key_file_size_in_byte);
    std::transform(i64_key_vec.begin(), i64_key_vec.end(), key_vec.begin(),
                   [](long long key) { return static_cast<unsigned>(key); });
  }
}

inline std::vector<char> load_to_vector(const std::string& file_name) {
  std::ifstream stream(file_name, std::ifstream::binary);
  if (!stream.is_open()) {
    CK_THROW_(Error_t::FileCannotOpen, "Can not open " + file_name);
  }

  size_t file_size_in_byte = fs::file_size(file_name);

  std::vector<char> vec(file_size_in_byte);
  stream.read(vec.data(), file_size_in_byte);
  return vec;
}

template <typename KeyType, typename EmbeddingCompType>
std::unique_ptr<IEmbedding> init_embedding(
    const SparseTensors<KeyType> &train_sparse_tensors,
    const SparseTensors<KeyType> &evaluate_sparse_tensors,
    const SparseEmbeddingHashParams &embedding_params,
    const std::shared_ptr<ResourceManager> &resource_manager,
    const Embedding_t embedding_type) {
  if (embedding_type == Embedding_t::DistributedSlotSparseEmbeddingHash) {
    std::unique_ptr<IEmbedding> embedding(
        new DistributedSlotSparseEmbeddingHash<KeyType, EmbeddingCompType>(
            train_sparse_tensors, evaluate_sparse_tensors,
            embedding_params, resource_manager));
    return embedding;
  } else {
    std::unique_ptr<IEmbedding> embedding(
        new LocalizedSlotSparseEmbeddingHash<KeyType, EmbeddingCompType>(
            train_sparse_tensors, evaluate_sparse_tensors,
            embedding_params, resource_manager));
    return embedding;
  }
}

inline void copy_sparse_model(
    std::string sparse_model_src, std::string sparse_model_dst) {
  if (fs::exists(sparse_model_dst)) {
    fs::remove_all(sparse_model_dst);
  }
  fs::copy(sparse_model_src, sparse_model_dst, fs::copy_options::recursive);
}

inline bool check_vector_equality(const char *sparse_model_src,
    const char *sparse_model_dst, const char *ext) {
  const std::string src_file(std::string(sparse_model_src) + "/" + ext);
  const std::string dst_file(std::string(sparse_model_dst) + "/" + ext);

  std::vector<char> vec_src = load_to_vector(src_file);
  std::vector<char> vec_dst = load_to_vector(dst_file);

  size_t len_src = vec_src.size();
  size_t len_dst = vec_src.size();

  if (len_src != len_dst) {
    return false;
  }
  bool flag = test::compare_array_approx<char>(vec_src.data(), vec_dst.data(), len_src, 0);
  return flag;
}

template <typename TypeKey, Check_t check>
inline void generate_sparse_model_impl(std::string sparse_model_name,
    std::string file_list_name_train, std::string file_list_name_eval,
    std::string prefix, int num_files, long long label_dim, long long dense_dim,
    size_t slot_num, int max_nnz_per_slot, size_t max_feature_num,
    size_t vocabulary_size, size_t emb_vec_size, int combiner, float scaler,
    int num_workers, size_t batchsize, int batch_num_train, int batch_num_eval,
    Update_t update_type, std::shared_ptr<ResourceManager> resource_manager) {
  // generate train/test datasets
  if (fs::exists(file_list_name_train)) {
    fs::remove(file_list_name_train);
  }

  if (fs::exists(file_list_name_eval)) {
    fs::remove(file_list_name_eval);
  }

  // data generation
  HugeCTR::data_generation_for_test<TypeKey, check>(file_list_name_train,
      prefix, num_files, batch_num_train * batchsize, slot_num,
      vocabulary_size, label_dim, dense_dim, max_nnz_per_slot);
  HugeCTR::data_generation_for_test<TypeKey, check>(file_list_name_eval,
      prefix, num_files, batch_num_eval * batchsize, slot_num,
      vocabulary_size, label_dim, dense_dim, max_nnz_per_slot);

  // create train/eval data readers
  const DataReaderSparseParam param = {
      "distributed",
      max_nnz_per_slot,
      false,
      static_cast<int>(slot_num)
  };
  std::vector<DataReaderSparseParam> data_reader_params;
  data_reader_params.push_back(param);

  std::unique_ptr<DataReader<TypeKey>> data_reader_train(
      new DataReader<TypeKey>(batchsize, label_dim, dense_dim,
          data_reader_params, resource_manager, true, num_workers, false));
  std::unique_ptr<DataReader<TypeKey>> data_reader_eval(
      new DataReader<TypeKey>(batchsize, label_dim, dense_dim,
          data_reader_params, resource_manager, true, num_workers, false));

  data_reader_train->create_drwg_norm(file_list_name_train, check);
  data_reader_eval->create_drwg_norm(file_list_name_eval, check);

  Embedding_t embedding_type = Embedding_t::LocalizedSlotSparseEmbeddingHash;

  // create an embedding
  OptHyperParams hyper_params;
  hyper_params.adam.beta1 = 0.9f;
  hyper_params.adam.beta2 = 0.999f;
  hyper_params.adam.epsilon = 1e-7f;
  hyper_params.momentum.factor = 0.9f;
  hyper_params.nesterov.mu = 0.9f;

  const OptParams opt_params = {Optimizer_t::Adam, 0.001f, hyper_params,
                                                   update_type, scaler};

  const SparseEmbeddingHashParams embedding_params = {
      batchsize,       batchsize, vocabulary_size, {},         emb_vec_size,
      max_feature_num, slot_num,  combiner,        opt_params};

  auto copy = [](const std::vector<SparseTensorBag> &tensorbags, SparseTensors<TypeKey> &sparse_tensors) {
    sparse_tensors.resize(tensorbags.size());
    for(size_t j = 0; j < tensorbags.size(); ++j){
      sparse_tensors[j] = SparseTensor<TypeKey>::stretch_from(tensorbags[j]);
    }
  };
  SparseTensors<TypeKey> train_inputs;
  copy(data_reader_train->get_sparse_tensors("distributed"), train_inputs);
  SparseTensors<TypeKey> eval_inputs;
  copy(data_reader_eval->get_sparse_tensors("distributed"), eval_inputs);

  auto embedding = init_embedding<TypeKey, float>(
      train_inputs,
      eval_inputs,
      embedding_params, resource_manager, embedding_type);
  embedding->init_params();

  // train the embedding
  data_reader_train->read_a_batch_to_device();
  embedding->forward(true);
  embedding->backward();
  embedding->update_params();

  // store the snapshot from the embedding
  embedding->dump_parameters(sparse_model_name);
}

template <typename TypeKey, Check_t check>
inline void generate_sparse_model(
    std::string snapshot_src_file,  std::string snapshot_dst_file,
    std::string sparse_model_unsigned, std::string sparse_model_longlong,
    std::string file_list_name_train, std::string file_list_name_eval,
    std::string prefix, int num_files, long long label_dim, long long dense_dim,
    size_t slot_num, int max_nnz_per_slot, size_t max_feature_num,
    size_t vocabulary_size, size_t emb_vec_size, int combiner, float scaler,
    int num_workers, size_t batchsize, int batch_num_train, int batch_num_eval,
    Update_t update_type, std::shared_ptr<ResourceManager> resource_manager) {
  if (std::is_same<TypeKey, unsigned>::value) {
    if (fs::exists(sparse_model_unsigned)) {
      if (fs::exists(snapshot_src_file)) fs::remove_all(snapshot_src_file);
      if (fs::exists(snapshot_dst_file)) fs::remove_all(snapshot_dst_file);
      copy_sparse_model(sparse_model_unsigned, snapshot_src_file);
    } else {
      generate_sparse_model_impl<unsigned, check>(snapshot_src_file,
          file_list_name_train, file_list_name_eval, prefix, num_files, label_dim,
          dense_dim, slot_num, max_nnz_per_slot, max_feature_num,
          vocabulary_size, emb_vec_size, combiner, scaler, num_workers, batchsize,
          batch_num_train, batch_num_eval, update_type, resource_manager);
      copy_sparse_model(snapshot_src_file, sparse_model_unsigned);
    }
  } else {
    if (fs::exists(sparse_model_longlong)) {
      if (fs::exists(snapshot_src_file)) fs::remove_all(snapshot_src_file);
      if (fs::exists(snapshot_dst_file)) fs::remove_all(snapshot_dst_file);
      copy_sparse_model(sparse_model_longlong, snapshot_src_file);
    } else {
      generate_sparse_model_impl<long long, check>(snapshot_src_file,
          file_list_name_train, file_list_name_eval, prefix, num_files, label_dim,
          dense_dim, slot_num, max_nnz_per_slot, max_feature_num,
          vocabulary_size, emb_vec_size, combiner, scaler, num_workers, batchsize,
          batch_num_train, batch_num_eval, update_type, resource_manager);
      copy_sparse_model(snapshot_src_file, sparse_model_longlong);
    }
  }
}

}

}