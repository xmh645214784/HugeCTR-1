"""
 Copyright (c) 2021, NVIDIA CORPORATION.
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
"""

import argparse

import sys, os
sys.path.append("../../") # where to find plugin
import sparse_operation_kit as sok
import tensorflow as tf

from test_sparse_emb_demo_model_single_worker import check_saved_embedding_variables

import pickle
import utils

class SOKDenseDemo(tf.keras.models.Model):
    def __init__(self, 
                 max_vocabulary_size_per_gpu,
                 embedding_vec_size,
                 slot_num, 
                 nnz_per_slot,
                 **kwargs):
        super(SOKDenseDemo, self).__init__(**kwargs)

        self.max_vocabulary_size_per_gpu = max_vocabulary_size_per_gpu
        self.slot_num = slot_num
        self.nnz_per_slot = nnz_per_slot
        self.embedding_vec_size = embedding_vec_size

        self.embedding_layer = sok.All2AllDenseEmbedding(max_vocabulary_size_per_gpu=self.max_vocabulary_size_per_gpu,
                                                         embedding_vec_size=self.embedding_vec_size,
                                                         slot_num=self.slot_num,
                                                         nnz_per_slot=self.nnz_per_slot)
        
        self.dense_layer = tf.keras.layers.Dense(units=1, activation=None,
                                                 kernel_initializer="ones",
                                                 bias_initializer="zeros")

    def call(self, inputs, training=True):
        # [batchsize, slot_num, nnz_per_slot, embedding_vec_size]
        embedding_vector = self.embedding_layer(inputs, training=training)
        # [batchsize, slot_num * nnz_per_slot * embedding_vec_size]
        embedding_vector = tf.reshape(embedding_vector, shape=[-1, self.slot_num * self.nnz_per_slot * self.embedding_vec_size])
        # [batchsize, 1]
        logit = self.dense_layer(embedding_vector)
        return logit, embedding_vector

class TfDenseDemo(tf.keras.models.Model):
    def __init__(self,
                 init_tensors,
                 global_batch_size,
                 slot_num, 
                 nnz_per_slot,
                 embedding_vec_size,
                 **kwargs):
        super(TfDenseDemo, self).__init__(**kwargs)
        self.init_tensors = init_tensors
        self.global_batch_size = global_batch_size
        self.slot_num = slot_num
        self.nnz_per_slot = nnz_per_slot
        self.embedding_vec_size = embedding_vec_size

        self.params = tf.Variable(initial_value=tf.concat(self.init_tensors, axis=0))

        self.dense_layer = tf.keras.layers.Dense(units=1, activation=None,
                                                 kernel_initializer="ones",
                                                 bias_initializer="zeros")

    def call(self, inputs, training=True):
        # [batchsize * slot_num * nnz_per_slot, embedding_vec_size]
        embedding_vector = tf.nn.embedding_lookup(params=self.params,
                                                  ids=inputs)

        # [batchsize, slot_num * nnz_per_slot * embedding_vec_size]
        embedding_vector = tf.reshape(embedding_vector, 
            shape=[self.global_batch_size, self.slot_num * self.nnz_per_slot * self.embedding_vec_size])
        
        # [batchsize, 1]
        logit = self.dense_layer(embedding_vector)
        return logit, embedding_vector

def test_sok_dense_demo(args, init_tensors, *random_samples):
    strategy = tf.distribute.MirroredStrategy()
    with strategy.scope():
        result = sok.Init(global_batch_size=args.global_batch_size)

        sok_dense_demo = SOKDenseDemo(max_vocabulary_size_per_gpu=args.max_vocabulary_size_per_gpu,
                                      embedding_vec_size=args.embedding_vec_size,
                                      slot_num=args.slot_num,
                                      nnz_per_slot=args.nnz_per_slot)
        emb_opt = utils.get_embedding_optimizer(args.optimizer)(learning_rate=0.1)
        dense_opt = utils.get_dense_optimizer(args.optimizer)(learning_rate=0.1)

    sok_saver = sok.Saver()

    if 1 == args.restore_params:
        filepath = r"./embedding_variables"
        sok_saver.restore_from_file(sok_dense_demo.embedding_layer.embedding_variable, filepath)
    else:
        sok_saver.load_embedding_values(sok_dense_demo.embedding_layer.embedding_variable, init_tensors)

    loss_fn = tf.keras.losses.BinaryCrossentropy(from_logits=True, reduction=tf.keras.losses.Reduction.NONE)
    def _replica_loss(labels, logits):
        loss = loss_fn(labels, logits)
        return tf.nn.compute_average_loss(loss, global_batch_size=args.global_batch_size)

    @tf.function
    def _train_step(inputs, labels):
        with tf.GradientTape() as tape:
            logit, embedding_vector = sok_dense_demo(inputs, training=True)
            loss = _replica_loss(labels, logit)
        embedding_variables, other_variable = sok.split_embedding_variable_from_others(sok_dense_demo.trainable_variables)
        grads, emb_grads = tape.gradient(loss, [other_variable, embedding_variables])
        if "plugin" not in args.optimizer:
            with sok.OptimizerScope(embedding_variables):
                emb_opt.apply_gradients(zip(emb_grads, embedding_variables),
                                        experimental_aggregate_gradients=False)
        else:
            emb_opt.apply_gradients(zip(emb_grads, embedding_variables),
                                    experimental_aggregate_gradients=False)
        dense_opt.apply_gradients(zip(grads, other_variable))
        return logit, embedding_vector

    def _dataset_fn(input_context):
        replica_batch_size = input_context.get_per_replica_batch_size(args.global_batch_size)
        dataset = utils.tf_dataset(*random_samples, batchsize=replica_batch_size, 
                                    to_sparse_tensor=False, repeat=1)
        dataset = dataset.shard(input_context.num_input_pipelines, input_context.input_pipeline_id)
        return dataset

    dataset = strategy.distribute_datasets_from_function(_dataset_fn)

    sok_results = list()
    for i, (input_tensors, replica_labels) in enumerate(dataset):
        print("-" * 30, "step ", str(i), "-" * 30)
        logit, embedding_vector = strategy.run(_train_step, args=(input_tensors, replica_labels))
        print("[INFO]: embedding_vector\n", embedding_vector)
        sok_results.append(embedding_vector)

        # FIXME: when the forward computation is too fast, there
        # may exist some conficts with datareader, which cause the program hang.
        import time
        time.sleep(0.2) # seconds

    if 1 == args.save_params:
        filepath = r"./embedding_variables/"
        utils.try_make_dirs(filepath)

        sok_saver.dump_to_file(sok_dense_demo.embedding_layer.embedding_variable, filepath)

    return sok_results, sok_dense_demo.embedding_layer.embedding_variable.values[0].m_var_name

def test_tf_dense_model(args, init_tensors, *random_samples):
    dataset = utils.tf_dataset(*random_samples, batchsize=args.global_batch_size,
                                to_sparse_tensor=False, repeat=1)
    
    loss_fn = tf.keras.losses.BinaryCrossentropy(from_logits=True)
    
    tf_dense_demo = TfDenseDemo(init_tensors, args.global_batch_size, args.slot_num,
                                args.nnz_per_slot, args.embedding_vec_size)
    
    optimizer = utils.get_dense_optimizer(args.optimizer)(learning_rate=0.1)

    @tf.function
    def _train_step(inputs, labels):
        with tf.GradientTape() as tape:
            logit, embedding_vector = tf_dense_demo(inputs, training=True)
            loss = loss_fn(labels, logit)
        grads = tape.gradient(loss, tf_dense_demo.trainable_variables)
        optimizer.apply_gradients(zip(grads, tf_dense_demo.trainable_variables))
        return logit, embedding_vector

    tf_results = list()

    for i, (input_tensors, labels) in enumerate(dataset):
        print("-"*30, str(i), "-"*30)
        logit, embedding_vector = _train_step(input_tensors, labels)
        print("[INFO]: embedding_vector:\n", embedding_vector)
        tf_results.append(embedding_vector.numpy())

        # FIXME: because plugin sleepd, here is only used for 
        # simulate the same DNN structure. 
        import time
        time.sleep(0.2) # seconds

    if not hasattr(args, "task_id"):
        args.task_id = 0
    if 1 == args.save_params and args.task_id == 0:
        filepath = r"./embedding_variables/"
        utils.save_to_file(os.path.join(filepath, r"tf_variable.file"),
                           tf_dense_demo.params.numpy())

    return tf_results

def compare_dense_emb_sok_with_tf(args):
    if (args.global_batch_size % args.gpu_num != 0):
        raise ValueError("global_batch_size: %d is not divisible by gpu_num: %d"
                        %(args.global_batch_size, args.gpu_num))

    if args.generate_new_datas:
        random_samples = utils.generate_random_samples(num_of_samples=args.global_batch_size * args.iter_num,
                                                    vocabulary_size=args.gpu_num * args.max_vocabulary_size_per_gpu * 1,
                                                    slot_num=args.slot_num,
                                                    max_nnz=args.nnz_per_slot,
                                                    use_sparse_mask=False)
        utils.save_to_file(r"./random_samples.file", *random_samples)
    else:
        random_samples = utils.restore_from_file(r"./random_samples.file")

    if 1 == args.restore_params:
        filepath = r"./embedding_variables"

        # because we already checked the Variable consistency when saving.
        # so that here we can directly use TensorFlow Variable file to initialize
        # tf's variable.
        # FIXME: what if not all TensorFlow embedding vectors are used??
        tf_values_filename = os.path.join(filepath, r"tf_variable.file")
        init_tensors = utils.restore_from_file(tf_values_filename)
    else:
        init_tensors = utils.get_ones_tensor(max_vocab_size_per_gpu=args.max_vocabulary_size_per_gpu,
                                        embedding_vec_size=args.embedding_vec_size,
                                        num=args.gpu_num)

    sok_results, embedding_variable_name = test_sok_dense_demo(args, init_tensors, *random_samples)
    tf_results = test_tf_dense_model(args, init_tensors, *random_samples)

    if len(sok_results) != len(tf_results):
        raise ValueError("The length of sok results is not equal to that of tensorflow.")
    if len(sok_results) != args.iter_num:
        raise ValueError("The length of embedding vectors: %d is not equal to iteration number: %d."
                        %(len(sok_results), args.iter_num))

    if 1 == args.restore_params:
        tolerance = 1e-2
    else:
        tolerance = 1e-4

    for i, sok_vector in enumerate(sok_results):
        if args.gpu_num != 1:
            sok_vector = tf.stack(sok_vector.values, axis=0)
        tf.debugging.assert_near(tf.reshape(sok_vector, 
                                            shape=[-1, tf.shape(sok_vector)[-1]]),
                                tf_results[i],
                                atol=tolerance,
                                rtol=tolerance,
                                message="the values is not consistent on Iteration: %d" %i)

    print("\n[INFO]: For Dense Embedding Layer: with MirroredStrategy, the embedding vector obtained from " +\
          "sparse operation kit and TensorFlow are consistent for %d iterations."
          %args.iter_num)

    if 1 == args.save_params:
        check_saved_embedding_variables(args, embedding_variable_name)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='test demo model with single worker.')
    parser.add_argument('--gpu_num', type=int,
                        help='the number of GPUs used to do paralell training.',
                        required=False, default=8)
    parser.add_argument('--iter_num', type=int,
                        help='the number of testing iterations.',
                        required=False, default=100)
    parser.add_argument('--max_vocabulary_size_per_gpu', type=int,
                        required=False, default=128)
    parser.add_argument('--slot_num', type=int,
                        help='the number of feature fields',
                        required=False, default=1)
    parser.add_argument('--nnz_per_slot', type=int,
                        help='the number of keys in each slot',
                        required=False, default=1)
    parser.add_argument('--embedding_vec_size', type=int,
                        help='the dimention of embedding vector',
                        required=False, default=1)
    parser.add_argument('--global_batch_size', type=int, required=False, default=16)
    parser.add_argument('--optimizer', type=str,
                        help="use what optimizer",
                        required=False, default='plugin_adam',
                        choices=['plugin_adam', 'adam', 'sgd'])
    parser.add_argument('--generate_new_datas', type=int, choices=[0, 1],
                        help='whether to generate new random samples',
                        required=False, default=1)
    parser.add_argument('--save_params', type=int, choices=[0, 1],
                        help='whether to save the trained parameters.',
                        required=False, default=0)
    parser.add_argument('--restore_params', type=int, choices=[0, 1],
                        help='whether to restore from saved files. '+\
                             'By default, the testing program will generate random ' +\
                             'initial value to initialize trainable parameters '+\
                             'rather than restore trainable parameters from file.',
                        required=False, default=0)

    args = parser.parse_args()

    import os
    os.environ['CUDA_VISIBLE_DEVICES'] = ",".join([str(i) for i in range(args.gpu_num)])

    compare_dense_emb_sok_with_tf(args)