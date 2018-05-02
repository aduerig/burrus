import tensorflow as tf
import numpy as np

import os

os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3' 

def main():
    with tf.Session() as sess:
        # np.random.seed(0)
        # tf.set_random_seed(0)
        
        model_dir = get_model_dir('recent')
        # model_dir = get_model_dir('model_0')
        print('Using model at: ' + model_dir)

        batch_size = 5
        data_gen = get_data(batch_size, model_dir)

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

        for v in updaters:
            print(v)
            print('')

        for v in var:
            print(v)
            print(sess.run(v))
            print('')
        print('')


        # exit()

        all_calced_values = []
        all_true_values = []
        num_batches_to_go = 10
        # to_run = next(data_gen)
        for i in range(num_batches_to_go):
            to_run = next(data_gen)
            curr_batch_x = to_run[0]
            curr_batch_y_policy_labels = to_run[1]
            curr_batch_y_true_value = to_run[2]
            
            res = sess.run([policy_head_output, value_head_output], feed_dict={x_tensor: curr_batch_x, train_bool: False})

            policy_calced = res[0]
            value_calced = res[1]

            # print(policy_calced)
            # print(value_calced)
            # exit()

            print(curr_batch_y_true_value)
            print(value_calced)

            for j in range(policy_calced.shape[0]):
                print_board(curr_batch_x[j])
                
                print("true values:")
                print(curr_batch_y_policy_labels[j])
                print(curr_batch_y_true_value[j])

                print("in pass:")
                print(policy_calced[j])
                print(value_calced[j])

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



def print_board(b):
    to_print = []
    print("BOARD, BLACK TO MOVE")
    for i in range(64):
        if b[i+64] == 1:
            to_print.append('W')
        elif b[i] == 1:
            to_print.append('B')
        else:
            to_print.append('-')
    for i in range(8):
        print(' '.join(to_print[(i*8):(i*8)+8]))
    print('')




def get_model_dir(name):
    data_dir_name = 'data'
    data_dir = os.path.abspath(os.path.join(os.getcwd(), data_dir_name))
    
    if name == 'recent':
        newest_model = len(next(os.walk(data_dir))[1])-1
        return os.path.join(data_dir, 'model_' + str(newest_model))

    return os.path.join(data_dir, name)


def get_inf_batch_gens(data, size):
    # data is deterministic up to here
    sample_length = data[0].shape[0] # 40762
    curr = sample_length
    loop = 0

    if size == 0:
        print("get_inf_batch_gens: cant have batch size 0, quitting")
        exit(0)
    
    if sample_length == 0:
        print("get_inf_batch_gens: passed 0 samples, quitting")
        exit(0)
    
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
            continue
        x = data[0][curr:curr+size]
        q_vals = data[1][curr:curr+size]
        true_result = data[2][curr:curr+size]
        curr += size
        yield x, q_vals, true_result

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
                # Grabbing board states
                board1 = []
                stripped_line = f.readline().strip()
                splitted_arr = stripped_line.split(',')[:-1]
                for _j in splitted_arr:
                    board1.append(int(_j))
               
                board2 = []
                stripped_line = f.readline().strip()
                splitted_arr = stripped_line.split(',')[:-1]
                for _j in splitted_arr:
                    board2.append(int(_j))

                boards.append(board1+board2)
                
                # grabbing q_vals
                arr = []
                stripped_line = f.readline().strip()
                splitted_arr = stripped_line.split(',')[:-1]
                for _j in splitted_arr:
                    arr.append(float(_j))
                evals.append(arr)

                # grabbing final result
                stripped_line = f.readline().strip()
                results.append([int(stripped_line)])
    # state = 42
    # print(filename)
    # print(boards[state])
    # print(evals[state])
    # print(results[state])
    # print_board(boards[state])
    # exit()
    print("loaded {0} board states".format(len(boards)))
    return boards, evals, results


def get_data(size, old_model_dir):
    x_train = []
    y_policy_labels = []
    y_true_value = []
    model_count = len(next(os.walk('data'))[1])
    game_locs = []

    for i in range(max(0, model_count-15), model_count):
        curr_path = os.path.join('data', 'model_' + str(i), 'games', 'all_games.game')
        x,y,z = read_in_games(curr_path)
        x_train += x
        y_policy_labels += y
        y_true_value += z

    x_train = np.array(x_train)
    y_policy_labels = np.array(y_policy_labels)
    y_true_value = np.array(y_true_value)

    # print(x_train.shape) # (40762, 128)
    # print(y_policy_labels.shape) # (40762, 64)
    # print(y_true_value.shape) # (40762, 1)
    # exit()

    train = [x_train, y_policy_labels, y_true_value]
    return get_inf_batch_gens(train, size)


if __name__ == "__main__":
    main()




# a = np.arange(1, 15)
# b = np.arange(1, 15)
# c = np.arange(1, 15)

# rng_state = np.random.get_state()
# np.random.shuffle(a)
# np.random.set_state(rng_state)
# np.random.shuffle(b)
# np.random.set_state(rng_state)
# np.random.shuffle(c)

# print(a)
# print(b)
# print(c)
# exit()