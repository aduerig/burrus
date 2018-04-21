from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import numpy as np
import tensorflow as tf

import os
import random

os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3' 

GLOBAL_LEARNING_RATE = .2
GLOBAL_TRAINING_STEPS = 1000
GLOBAL_BATCH_SIZE = 64

MODELS_DIRECTORY = 'data' 

# may be training weirdness:
# extra_update_ops = tf.get_collection(tf.GraphKeys.UPDATE_OPS)
# sess.run([train_op, extra_update_ops], ...)
# may be needed to train batchnorm features


# to interact with the gpu, make sure the following are loaded
# slurm/default and psc_path/1.1
# then interact -gpu
# then module load tensorflow/1.5_gpu


# - conv bloc is:
#     - 128 filters of kernal size 3x3 with stride 1 (orig paper is 256)
#     - batch norm
#     - ReLU
#     - unsure of padding needed, though "same" is probably the intended padding method

# train_bool is set to True or False depending on if its in infrence or not 
#       (probably will always not be since infrence will be run in c++, not this model)
def cnn_block(x, train_bool, block_num):
    x = tf.reshape(x, [-1, 8, 8, 2])

    block_num = str(block_num)
    # conv
    conv_layer = tf.layers.conv2d(
            inputs=x,
            filters=128,
            kernel_size=[3, 3],
            padding="same",
            activation=None,
            name='cnn_block_' + block_num + '_conv_layer')

    # batchnorm
    conv_bn_layer = tf.layers.batch_normalization(
        inputs=conv_layer,
        axis=-1,
        momentum=0.9,
        epsilon=0.001,
        center=True,
        scale=True,
        training = train_bool,
        name='cnn_block_' + block_num + '_conv_bn_layer')

    # relu
    relu = tf.nn.relu(conv_bn_layer, name='cnn_block_' + block_num + '_relu')

    return relu

def resid_block(x, train_bool, block_num):
    block_num = str(block_num)
    # conv
    conv_layer = tf.layers.conv2d(
            inputs=x,
            filters=128,
            kernel_size=[3, 3],
            padding="same",
            activation=None,
            name='resid_block_' + block_num + '_conv_layer')

    # batchnorm
    conv_bn_layer = tf.layers.batch_normalization(
        inputs=conv_layer,
        axis=-1,
        momentum=0.9,
        epsilon=0.001,
        center=True,
        scale=True,
        training = train_bool,
        name='resid_block_' + block_num + '_conv_bn_layer')

    # inbetween relu
    first_relu = tf.nn.relu(conv_bn_layer)

    # conv2
    conv2_layer = tf.layers.conv2d(
            inputs=first_relu,
            filters=128,
            kernel_size=[3, 3],
            padding="same",
            activation=None,
            name='resid_block_' + block_num + '_conv2_layer')

    # batchnorm2
    conv2_bn_layer = tf.layers.batch_normalization(
        inputs=conv2_layer,
        axis=-1,
        momentum=0.9,
        epsilon=0.001,
        center=True,
        scale=True,
        training = train_bool,
        name='resid_block_' + block_num + '_conv2_bn_layer')

    # residual
    resid_connection = tf.add(conv2_bn_layer, x, name='resid_block_resid_connection')

    # final relu
    final_relu = tf.nn.relu(resid_connection, name='resid_block_relu')

    return final_relu


def create_value_head(x, train_bool):
    # conv
    conv_layer = tf.layers.conv2d(
            inputs=x,
            filters=1,
            kernel_size=[1, 1],
            padding="same",
            activation=None,
            name='value_head_conv_layer')

    # batchnorm
    conv_bn_layer = tf.layers.batch_normalization(
        inputs=conv_layer,
        axis=-1,
        momentum=0.9,
        epsilon=0.001,
        center=True,
        scale=True,
        training = train_bool,
        name='value_head_conv_bn_layer')

    # relu
    first_relu = tf.nn.relu(conv_bn_layer, name='value_head_relu1')


    flattened_value = tf.reshape(first_relu, [-1, 8*8])

    hidden_layer = tf.layers.dense(inputs=flattened_value, units = 256, name='value_head_dense_to_dense') # only 64 possible moves, no activation

    final_relu = tf.nn.relu(hidden_layer, name='value_head_relu2')

    board_value_not_capped = tf.layers.dense(inputs=final_relu, units = 1, name='value_head_dense_to_scaler') # only 64 possible moves, no activation

    board_value = tf.nn.tanh(board_value_not_capped, name='value_head_output')

    return board_value


def create_policy_head(x, train_bool):
    # conv
    conv_layer = tf.layers.conv2d(
            inputs=x,
            filters=2,
            kernel_size=[1, 1],
            padding="same",
            activation=None,
            name='policy_head_conv_layer')

    # batchnorm
    conv_bn_layer = tf.layers.batch_normalization(
        inputs=conv_layer,
        axis=-1,
        momentum=0.9,
        epsilon=0.001,
        center=True,
        scale=True,
        training = train_bool,
        name='policy_head_conv_bn_layer')

    # relu
    relu = tf.nn.relu(conv_bn_layer, name='policy_head_relu')

    flattened_policy = tf.reshape(relu, [-1, 2 * 8 * 8])

    move_space = tf.layers.dense(inputs=flattened_policy, units = 64, name='policy_head_output') # only 64 possible moves, no activation

    return move_space


# generators will loop forever if batch_size > samples, also it has the chance to miss a
# few samples each iteration, though they all have equal probability, so it shouldnt matter
def get_inf_batch_gens(data, size):
    rng_state = np.random.get_state()
    np.random.shuffle(data[0])
    np.random.set_state(rng_state)
    np.random.shuffle(data[1])
    np.random.set_state(rng_state)
    np.random.shuffle(data[2])
    sample_length = len(data[0])
    curr = sample_length
    loop = 0
    while True:
        if curr+size > sample_length:
            curr = 0
            rng_state = np.random.get_state()
            np.random.shuffle(data[0])
            np.random.set_state(rng_state)
            np.random.shuffle(data[1])
            np.random.set_state(rng_state)
            np.random.shuffle(data[2])
            loop += 1
            print('looping training data for the {0} time'.format(loop))
            continue
        x = data[0][curr:curr+size]
        y_real = data[1][curr:curr+size]
        y_imag = data[2][curr:curr+size]
        curr += size
        yield x, y_real, y_imag


def get_model_directories():
    # get newest model directory
    data_dir_name = 'data'
    data_dir = os.path.abspath(os.path.join(os.getcwd(), data_dir_name))
    
    # ensure data_dir exists
    if not os.path.isdir(data_dir):
        print('MODEL: making data directory at {0}', data_dir)
        os.mkdir(data_dir)

    newest_model = len(next(os.walk(data_dir))[1])
    older_model = newest_model - 1

    # if there is no models avaliable we must start from scratch
    if older_model == -1:
        print('MODEL: no data exists, saving network with random weights')
        return None, os.path.join(data_dir, 'model_' + str(newest_model))
    return os.path.join(data_dir, 'model_' + str(older_model)), os.path.join(data_dir, 'model_' + str(newest_model))


def get_data(size, old_model_dir):

    x_train = []
    y_policy_labels = []
    y_true_value = []
    model_count = len(next(os.walk('data'))[1])
    game_locs = []

    for i in range(model_count):
        curr_path = os.path.join('data', 'model_' + str(i), 'games', 'all_games.game')
        x,y,z = read_in_games(curr_path)
        x_train += x
        y_policy_labels += y
        y_true_value += z

    x_train = np.array(x_train)
    y_policy_labels = np.array(y_policy_labels)
    y_true_value = np.array(y_true_value)

    train = [x_train, y_policy_labels, y_true_value]
    print(train)
    input()
    return get_inf_batch_gens(train, size)

def bitfield(n):
    return [n >> i & 1 for i in range(63,-1,-1)]

def read_in_games(filename):
    boards = []
    evals = []
    results = []
    if not os.path.exists(filename):
        print("couldn't find games for path: " + filename)
        return boards, evals, results
    with open(filename, "r") as f:
        while True:
            move_count = f.readline()
            if not move_count:
                break
            for i in range(int(move_count)):
                board1 = bitfield(int(f.readline()))
                board2 = bitfield(int(f.readline()))
                boards.append(board1+board2)
                arr = []
                for _j in range(64):
                    arr.append(float(f.readline()))
                evals.append(arr)
                #skip saved values for now
                f.readline()
                results.append([int(f.readline())])
    return boards, evals, results

# returns 1 when successful
def train():
    concat_files()
    x = tf.placeholder(tf.float32, shape=(None, 128), name='x')
    train_bool = tf.placeholder(tf.bool, name='train_bool')
    y_policy_labels = tf.placeholder(tf.float32, shape=(None, 64), name='y_policy_labels')
    y_true_value = tf.placeholder(tf.float32, shape=(None, 1), name='y_true_value')

    conv_block = cnn_block(x, train_bool, 0)

    # stacking 10 residual blocks
    resid_input = conv_block
    for i in range(0, 10):
        resid_input = resid_block(resid_input, train_bool, i)
    resid_final = resid_input

    # get policy and value head
    value_head = create_value_head(resid_final, train_bool)
    policy_head = create_policy_head(resid_final, train_bool)


    # logits and labels must have the same shape, e.g. [batch_size, num_classes] and the same dtype (either float16, float32, or float64).
    # policy head loss
    policy_loss = tf.losses.softmax_cross_entropy(
            y_policy_labels, # labels
            policy_head, # logits
            weights=.5,
            label_smoothing=0,
            loss_collection=tf.GraphKeys.LOSSES,
            reduction=tf.losses.Reduction.SUM_BY_NONZERO_WEIGHTS)


    # value head loss
    value_loss = tf.losses.mean_squared_error(
            y_true_value, # label
            value_head, # prediction
            weights=1.0,
            scope=None,
            loss_collection=tf.GraphKeys.LOSSES,
            reduction=tf.losses.Reduction.SUM_BY_NONZERO_WEIGHTS)


    # combine
    # need to add l2 regularization
    total_loss = tf.add(policy_loss, value_loss, name='loss_combined')

    # toptimizer
    train_step = tf.train.MomentumOptimizer(learning_rate=GLOBAL_LEARNING_RATE, momentum=0.9, name='sgd').minimize(total_loss)
    correct_policy_prediction = tf.equal(tf.argmax(y_policy_labels, 1), tf.argmax(policy_head, 1))
    accuracy_policy = tf.reduce_mean(tf.cast(correct_policy_prediction, tf.float32))
    accuracy_value = tf.reduce_mean(tf.abs(value_head - y_true_value))

    # for training batchnorm features
    extra_update_ops = tf.get_collection(tf.GraphKeys.UPDATE_OPS)

    # create a saver (could need different arg passed)
    saver = tf.train.Saver(tf.trainable_variables())

    # initialize the graph
    init = tf.global_variables_initializer()

    # training
    with tf.Session(config=tf.ConfigProto(log_device_placement=True)) as sess:
        sess.run(init)

        old_model_dir, new_model_dir = get_model_directories()
        # No data exists, save random weights to be used in datagen
        if old_model_dir == None:
            os.mkdir(new_model_dir)
            os.mkdir(os.path.join(new_model_dir, 'games'))
            print(new_model_dir)
            saver.save(sess, os.path.join(new_model_dir, 'model.ckpt'))
            return 1
            
        saver.restore(sess, os.path.join(old_model_dir, 'model.ckpt'))
        train_batch_gen= get_data(GLOBAL_BATCH_SIZE, old_model_dir)


        for i in range(GLOBAL_TRAINING_STEPS):
            curr_batch_holder = next(train_batch_gen)
            curr_batch_x = curr_batch_holder[0]
            curr_batch_y_policy_labels = curr_batch_holder[1]
            curr_batch_y_true_value = curr_batch_holder[2]
            if i % 100 == 0:
                a_p = accuracy_policy.eval(feed_dict={
                        x: curr_batch_x, y_policy_labels: curr_batch_y_policy_labels,
                            train_bool: True})
                a_v = accuracy_value.eval(feed_dict={
                        x: curr_batch_x, y_true_value: curr_batch_y_true_value,
                            train_bool: True})
                print('step {0}, training accuracy_policy {1}, training accuracy_value {2}'.format(i, 
                            a_p, a_v))
            sess.run([train_step, extra_update_ops], feed_dict={x: curr_batch_x, 
                            y_policy_labels: curr_batch_y_policy_labels, 
                            y_true_value: curr_batch_y_true_value,
                            train_bool: True})

            if i % 20000:
                pass

        os.mkdir(new_model_dir)
        os.mkdir(os.path.join(new_model_dir, 'games'))
        saver.save(sess, os.path.join(new_model_dir, 'mdel.ckpt'))
    return 1

def concat_files():
    out_filename = 'all_games.game'

    model_count = len(next(os.walk(MODELS_DIRECTORY))[1])-1
    latest_model_path = 'model_' + str(model_count)
    path = os.path.join(MODELS_DIRECTORY, latest_model_path, 'games')
    if not os.path.isdir(MODELS_DIRECTORY) or os.path.isdir(path):
        return
    if os.path.exists(os.path.join(path, out_filename)):
        return
    with open(os.path.join(path, out_filename), 'wb+') as outfile:
        for file in os.listdir(path):
            if file == out_filename or file[-5:] != '.game':
                continue
            with open(os.path.join(path, file), 'rb') as readfile:
                outfile.write(readfile.read())

def main():
    train()

if __name__ == "__main__":
    main()