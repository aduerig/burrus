from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import numpy as np
import tensorflow as tf

import os

tf.logging.set_verbosity(tf.logging.INFO)


GLOBAL_LEARNING_RATE = .2
GLOBAL_TRAINING_STEPS = 100
GLOBAL_BATCH_SIZE = 64


# may be training weirdness:
# extra_update_ops = tf.get_collection(tf.GraphKeys.UPDATE_OPS)
# sess.run([train_op, extra_update_ops], ...)
# may be needed to train batchnorm features



# - conv bloc is:
#     - 128 filters of kernal size 3x3 with stride 1 (orig paper is 256)
#     - batch norm
#     - ReLU
#     - unsure of padding needed, though "same" is probably the intended padding method

# train_bool is set to True or False depending on if its in infrence or not 
#       (probably will always not be since infrence will be run in c++, not this model)
def cnn_block(x, train_bool, block_num):
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
    conv_bn_layer = tf.nn.relu(conv1_bn)

    return conv_bn_layer

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
        inputs=conv_layer2,
        axis=-1,
        momentum=0.9,
        epsilon=0.001,
        center=True,
        scale=True,
        training = train_bool,
        name='resid_block_' + block_num + '_conv2_bn_layer')

    # residual
    resid_connection = tf.add(conv2_bn_layer, x)

    # final relu
    final_relu = tf.nn.relu(resid_connection, name='policy_head_output')

    return final_relu


def value_head(x, train_bool):
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

    hidden_layer = tf.layers.dense(inputs=first_relu, units = 256, , name='value_head_dense_to_dense') # only 64 possible moves, no activation

    final_relu = tf.nn.relu(hidden_layer, name='value_head_relu2')

    board_value_not_capped = tf.layers.dense(inputs=final_relu, units = 1, name='value_head_dense_to_scaler') # only 64 possible moves, no activation

    board_value = tf.nn.tanh(board_value_not_capped, name='value_head_output')

    return board_value


def policy_head(x, train_bool):
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

    move_space = tf.layers.dense(inputs=relu, units = 64, name='policy_head_output') # only 64 possible moves, no activation

    return move_space


# generators will loop forever if batch_size > samples, also it has the chance to miss a
# few samples each iteration, though they all have equal probability, so it shouldnt matter
def get_inf_batch_gens(data, size):
    np.random.shuffle(data)
    sample_length = data.shape[0]
    curr = sample_length
    loop = 0
    while True:
        if curr+size > sample_length:
            curr = 0
            np.random.shuffle(data)
            loop += 1
            print('looping training data for the {0} time'.format(loop))
            continue
        x = data[curr:curr+size, 0]
        y_real = data[curr:curr+size, 1]
        y_imag = data[curr:curr+size, 2]
        curr += size
        yield x, y_real, y_imag


def get_model_directories():
    # get newest model directory
    data_dir_name = 'data'
    data_dir = os.path.abspath(os.path.join(os.getcwd(), os.pardir, data_dir_name))
    
    # ensure data_dir exists
    if not os.path.isdir(data_dir):
        os.mkdir(data_dir)

    newest_model = len(next(os.walk('dir_name'))[1])
    older_model = newest_model - 1

    # if there is no models avaliable we must start from scratch
    if newest_model == -1:
        return None, os.path.join(data_dir, 'model_' + str(newest_model))
    return os.path.join(data_dir, 'model_' + str(older_model)), os.path.join(data_dir, 'model_' + str(newest_model))


def get_data(size, old_model_dir):
    # open data
    x_path = os.path.join(old_model_dir, 'board_positions')
    y_policy_labels_path = os.path.join(old_model_dir, 'y_policy_labels')
    y_true_value_path = os.path.join(old_model_dir, 'y_true_value')

    x = np.loadtxt(open(x_path, 'r'), delimiter=',')
    y_policy_labels = np.loadtxt(open(y_policy_labels_path, 'r'), delimiter=',')
    y_true_value = np.loadtxt(open(y_true_value_path, 'r'), delimiter=',')
    
    # splits data into train/test, probably not needed for real training but for test purposes
    # possible these ar eshallow copies, check
    tt_split = x.shape[0] * .9

    x_train = x[:tt_split]
    y_policy_labels_train = y_policy_labels[:tt_split]
    y_true_value_train = y_true_value[:tt_split]

    x_test = x[tt_split:]
    y_policy_labels_test = y_policy_labels[tt_split:]
    y_true_value_test = y_true_value[tt_split:]

    # combining all training data in order to make it easier to shuffle
    all_train = np.empty(shape=[3, x_train.shape[0], x_train.shape[1]], 
            dtype=x_train.dtype)
    all_train[0] = x_train
    all_train[1] = y_policy_labels_train
    all_train[2] = y_true_value_train
    all_train = all_train.swapaxes(0, 1)
    print('Number of training examples', all_train.shape[0])

    # combining all testing data in order to make it easier to shuffle
    all_test = np.empty(shape=[3, x_test.shape[0], x_test.shape[1]], 
            dtype=x_test.dtype)
    all_test[0] = x_test
    all_test[1] = y_policy_labels_test
    all_test[2] = y_true_value_test
    all_test = all_test.swapaxes(0, 1)
    print('Number of test examples', all_test.shape[0])

    assert(all_train.shape[0] >= size)
    assert(all_test.shape[0] >= size)
    return get_inf_batch_gens(all_train, size), get_inf_batch_gens(all_test, size)


# returns 1 when successful
def main():
    x = tf.placeholder(tf.float32, shape=(None, 128), name='x')
    y_policy_labels = tf.placeholder(tf.float32, shape=(None, 64), name='y_policy_labels')
    y_true_value = tf.placeholder(tf.float32, shape=(None, 1), name='y_true_value')

    conv_block = cnn_block(x, True, 0)

    # stacking 10 residual blocks
    resid_input = conv_block
    for i in range(0, 10):
        resid_input = resid_block(resid_input, True, i)
    resid_final = resid_input

    # get policy and value head
    value_head = value_head(resid_final, True)
    policy_head = policy_head(resid_final, True)

    # logits and labels must have the same shape, e.g. [batch_size, num_classes] and the same dtype (either float16, float32, or float64).
    # policy head loss
    policy_loss = tf.losses.softmax_cross_entropy(
            labels=y_policy_labels,
            logits=policy_head,
            weights=.5,
            label_smoothing=0,
            loss_collection=tf.GraphKeys.LOSSES,
            reduction=Reduction.SUM_BY_NONZERO_WEIGHTS,
            name='loss_softmax_cross_entropy_with_logits')


    # value head loss
    value_loss = tf.losses.mean_squared_error(
            labels=y_true_value,
            logits=value_head,
            weights=1.0,
            scope=None,
            loss_collection=tf.GraphKeys.LOSSES,
            reduction=Reduction.SUM_BY_NONZERO_WEIGHTS,
            name='loss_mean_squared_error')


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
    init = tf.initialize_all_variables()

    # training
    with tf.Session() as sess:
        # sess.run(tf.global_variables_initializer())
        sess.run(init)

        old_model_dir, new_model_dir = get_model_directory()
        # No data exists, save random weights to be used in datagen
        if old_model_dir == None:
            os.mkdir(new_model_dir)
            saver.save(sess, new_model_dir)
            return 1

        train_batch_gen, test_batch_gen = get_data(GLOBAL_BATCH_SIZE, old_model_dir)


        for i in range(GLOBAL_TRAINING_STEPS):
            curr_batch_holder = next(train_batch_gen)
            curr_batch_x = curr_batch_holder[0]
            curr_batch_y_policy_labels = curr_batch_holder[1]
            curr_batch_y_true_value = curr_batch_holder[2]
            if i % 100 == 0:
                a_p = accuracy_policy.eval(feed_dict={
                        x: curr_batch_x, y_policy_labels: curr_batch_y_policy_labels})
                a_v = accuracy_policy.eval(feed_dict={
                        x: curr_batch_x, y_true_value: curr_batch_y_true_value})
                print('step {0}, training accuracy_policy {1}, training accuracy_value {1}'.format(i, 
                            a_p, a_v))
            sess.run([train_step, extra_update_ops], feed_dict={x: curr_batch_x, 
                            y_policy_labels: curr_batch_y_policy_labels, 
                            y_true_value: curr_batch_y_true_value})

            if i % 20000:
                pass

        curr_batch_holder = next(test_batch_gen)
        a_p = accuracy_policy.eval(feed_dict={
                x: curr_batch_x, y_policy_labels: curr_batch_y_policy_labels})
        a_v = accuracy_policy.eval(feed_dict={
                x: curr_batch_x, y_true_value: curr_batch_y_true_value})
        print('step {0}, testing accuracy_policy {1}, testing accuracy_value {1}'.format(i, 
                    a_p, a_v))
    os.mkdir(new_model_dir)
    saver.save(sess, new_model_dir)
    return 1


if __name__ == "__main__":
    main()