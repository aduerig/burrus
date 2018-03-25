from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import numpy as np
import tensorflow as tf

tf.logging.set_verbosity(tf.logging.INFO)


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
def cnn_block(x, train_bool):
    # conv
    conv_layer = tf.layers.conv2d(
            inputs=x,
            filters=128,
            kernel_size=[3, 3],
            padding="same",
            activation=None,
            name='conv_layer')

    # batchnorm
    conv_bn_layer = tf.layers.batch_normalization(
        inputs=conv_layer,
        axis=-1,
        momentum=0.9,
        epsilon=0.001,
        center=True,
        scale=True,
        training = train_bool,
        name='conv_bn_layer'
    )

    # relu
    conv_bn_layer = tf.nn.relu(conv1_bn)

    return conv_bn_layer

def resid_block(x, train_bool):
    # conv
    conv_layer = tf.layers.conv2d(
            inputs=x,
            filters=128,
            kernel_size=[3, 3],
            padding="same",
            activation=None,
            name='conv_layer')

    # batchnorm
    conv_bn_layer = tf.layers.batch_normalization(
        inputs=conv_layer,
        axis=-1,
        momentum=0.9,
        epsilon=0.001,
        center=True,
        scale=True,
        training = train_bool,
        name='conv_bn_layer'
    )

    # inbetween relu
    first_relu = tf.nn.relu(conv_bn_layer)

    # conv2
    conv2_layer = tf.layers.conv2d(
            inputs=first_relu,
            filters=128,
            kernel_size=[3, 3],
            padding="same",
            activation=None,
            name='conv2_layer')

    # batchnorm2
    conv2_bn_layer = tf.layers.batch_normalization(
        inputs=conv_layer2,
        axis=-1,
        momentum=0.9,
        epsilon=0.001,
        center=True,
        scale=True,
        training = train_bool,
        name='conv2_bn_layer'
    )

    # residual
    resid_connection = tf.add(conv2_bn_layer, x)

    # final relu
    final_relu = tf.nn.relu(resid_connection)

    return final_relu
