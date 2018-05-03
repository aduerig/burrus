import tensorflow as tf
import numpy as np
import utils
import os
import time

os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3' 

def main():
    with tf.Session() as sess:
        utils.concat_files()
        # np.random.seed(0)
        # tf.set_random_seed(0)
        
        model_dir = utils.get_model_dir('recent')
        if model_dir == None:
            print("could not find model, exiting...")
            exit()
        # model_dir = utils.get_model_dir('model_0')
        print('Using model at: ' + model_dir)

        batch_size = 1

        # t1 = time.time()
        data_gen = utils.get_data(batch_size, 15)
        # t2 = time.time()

        # print("time taken: {0}".format(t2-t1))
        # exit()

        saver = tf.train.import_meta_graph(os.path.join(model_dir, 'model.ckpt.meta'))
        graph = tf.get_default_graph()

        x_tensor = graph.get_tensor_by_name('x:0')
        train_bool = train_bool = graph.get_tensor_by_name('train_bool:0')
        value_head_output = graph.get_tensor_by_name('value_head_output:0')
        policy_head_output = graph.get_tensor_by_name('policy_head_output:0')

        saver.restore(sess, os.path.join(model_dir, 'model.ckpt'))
        # sess.run(tf.global_variables_initializer())

        updaters = tf.get_collection(tf.GraphKeys.UPDATE_OPS)
        var = tf.trainable_variables()

        # for v in updaters:
        #     print(v)
        #     print('')

        # for v in var:
        #     print(v)
        #     print(sess.run(v))
        #     print('')
        # print('')

        x_reshaped_test = tf.reshape(x_tensor, [-1, 8, 8, 2])


        all_calced_values = []
        all_true_values = []
        num_batches_to_go = 10
        # to_run = next(data_gen)
        for i in range(num_batches_to_go):
            to_run = next(data_gen)
            curr_batch_x = to_run[0]
            curr_batch_y_policy_labels = to_run[1]
            curr_batch_y_true_value = to_run[2]

            res = sess.run([x_reshaped_test], 
                                feed_dict={x_tensor: curr_batch_x})

            print(res[0][0])
            print(res[0].shape)
            print(res[0][0].shape)
            exit()
            
            # res = sess.run([policy_head_output, value_head_output], 
            #                     feed_dict={x_tensor: curr_batch_x, train_bool: False})

            # policy_calced = res[0]
            # value_calced = res[1]

            # print(curr_batch_y_true_value)
            # print(value_calced)

            # for j in range(policy_calced.shape[0]):
            #     utils.print_board(curr_batch_x[j])
                
            #     print("true values:")
            #     print(curr_batch_y_policy_labels[j])
            #     print(curr_batch_y_true_value[j])

            #     print("in pass:")
            #     print(policy_calced[j])
            #     print(value_calced[j])

                # print("true:", curr_batch_y_true_value[j][0], "predicted:", value_calced[j][0])


                # all_true_values.append(curr_batch_y_true_value[j][0])
                # all_calced_values.append(value_calced[j][0])
            
            # all_true_values.append(curr_batch_y_true_value[0][0])
            # all_calced_values.append(value_calced[0][0])


        # print(sorted(all_calced_values))

        # print('calced')
        # neg = [i for i in all_calced_values if i < 0]
        # pos = [i for i in all_calced_values if i > 0]
        # print('neg', len(neg))
        # print('pos', len(pos))

        # print('true')
        # neg = [i for i in all_true_values if i < 0]
        # pos = [i for i in all_true_values if i > 0]
        # print('neg', len(neg))
        # print('pos', len(pos))


        # together = np.append(policy_calced, value_calced)
        # print(together)

        # print(tf.trainable_variables())
        # print(GraphKeys.TRAINABLE_VARIABLES)
        utils.count_identical_board_pos()


if __name__ == "__main__":
    main()