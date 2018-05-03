import os
import numpy as np
import time

MAIN_DATA_DIRECTORY = 'data'

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
        newest_model = len(os.listdir(data_dir))-1
        return os.path.join(data_dir, 'model_' + str(newest_model))

    return os.path.join(data_dir, name)


def get_games_dir(name):
    return os.path.join(get_model_dir(name), 'games')


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
                stripped_line = f.readline()
                splitted_arr = stripped_line.split(',')
                for _j in splitted_arr:
                    board1.append(int(_j))
               
                board2 = []
                stripped_line = f.readline()
                splitted_arr = stripped_line.split(',')
                for _j in splitted_arr:
                    board2.append(int(_j))

                boards.append(board1+board2)
                
                # grabbing move_made
                stripped_line = f.readline()
                move_made = int(stripped_line)
                if move_made == -1:
                    f.readline()
                    f.readline()
                    continue

                # grabbing q_vals
                arr = []
                stripped_line = f.readline()
                splitted_arr = stripped_line.split(',')
                for _j in splitted_arr:
                    arr.append(float(_j))
                evals.append(arr)

                # grabbing final result
                stripped_line = f.readline()
                results.append([int(stripped_line)])
    print("loaded {0} board states".format(len(boards)))
    return boards, evals, results


def read_in_games_split_moves(filename):
    if not os.path.exists(filename):
        print("couldn't find games for path: " + filename)
        return boards, evals, results
    game_holder = []
    with open(filename, "r") as f:
        while True:
            move_count = f.readline()
            if not move_count:
                break            
            boards = []
            evals = []
            results = []
            
            for i in range(int(move_count)):
                # Grabbing board states
                board1 = []
                stripped_line = f.readline()
                splitted_arr = stripped_line.split(',')
                for _j in splitted_arr:
                    board1.append(int(_j))
               
                board2 = []
                stripped_line = f.readline()
                splitted_arr = stripped_line.split(',')
                for _j in splitted_arr:
                    board2.append(int(_j))

                boards.append(board1 + board2)
                
                # grabbing move_made
                stripped_line = f.readline()
                move_made = int(stripped_line)
                if move_made == -1:
                    f.readline()
                    f.readline()
                    continue

                # grabbing q_vals
                arr = []
                stripped_line = f.readline()
                splitted_arr = stripped_line.split(',')
                for _j in splitted_arr:
                    arr.append(float(_j))
                evals.append(arr)

                # grabbing final result
                stripped_line = f.readline()
                results.append([int(stripped_line)])
            game_holder.append([list(reversed(boards)), list(reversed(evals)), list(reversed(results))])
    print("loaded {0} games".format(len(game_holder)))
    return game_holder


def concat_files():
    out_filename = 'all_games.game'
    if not os.path.isdir(MAIN_DATA_DIRECTORY):
        return
    model_count = len(os.listdir(MAIN_DATA_DIRECTORY))-1
    latest_model_path = 'model_' + str(model_count)
    path = os.path.join(MAIN_DATA_DIRECTORY, latest_model_path, 'games')
    
    # checking that directory exists
    if not os.path.isdir(path):
        return

    # And that files are in the directory
    directories = os.listdir(path)
    if len(directories) == 0:
        return

    # checking if combined file already exists
    if os.path.exists(os.path.join(path, out_filename)):
        return
    
    # combining all
    with open(os.path.join(path, out_filename), 'wb+') as outfile:
        for file in os.listdir(path):
            if file == out_filename or file[-5:] != '.game':
                continue
            with open(os.path.join(path, file), 'rb') as readfile:
                outfile.write(readfile.read())

    # deleting useless files
    for p in directories:
        filename, file_extension = os.path.splitext(p)
        if file_extension == '.timers' or filename == "all_games":
            continue
        # print(os.path.join(path, p))
        os.remove(os.path.join(path, p))
    exit()



def u64_to_array(n):
    return [n >> i & 1 for i in range(63, -1, -1)]



# generators will loop forever if batch_size > samples, also it has the chance to miss a
# few samples each iteration, though they all have equal probability, so it shouldnt matter
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


def get_data(size, num_models_reach_back):
    x_train = []
    y_policy_labels = []
    y_true_value = []
    model_count = len(next(os.walk('data'))[1])
    game_locs = []

    for i in range(max(0, model_count-num_models_reach_back), model_count):
        curr_path = os.path.join('data', 'model_' + str(i), 'games', 'all_games.game')

        # t1 = time.time()
        x,y,z = read_in_games(curr_path)
        # t2 = time.time()

        # print("subtime taken: {0}".format(t2-t1))


        x_train += x
        y_policy_labels += y
        y_true_value += z

    x_train = np.array(x_train)
    y_policy_labels = np.array(y_policy_labels)
    y_true_value = np.array(y_true_value)

    train = [x_train, y_policy_labels, y_true_value]
    return get_inf_batch_gens(train, size)