# Python modules
import mmap
import os
import sys
import hashlib
import struct
import time
import argparse
import os

# 3rd party modules
import posix_ipc
import tensorflow as tf
import numpy as np


# THIS SCRIPT REQUIRES NUMPY > 1.9
# module load tensorflow/1.5_gpu loads an acceptable version of numpy

# THIS SCRIPT ALSO REQUIRES posix_ipc
# run pip insall --user posix_ipc to get it installed

# bridges runs this script 2x slower than my computer (9 seconds for 500000 iterations)


os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3' # not sure about this



def get_floats():
    arr = np.arange(1, 65, dtype=np.float32)
    for i in range(len(arr)):
        arr[i] = arr[i] / len(arr)
    return arr


def write_send_code(mapfile, write_file): # writing correct code back
    mapfile.seek(0)
    mapfile.write("\x01\x00\x00\x00")


def write_to_memory(mapfile, s):
    mapfile.seek(4)
    # write rest of string
    mapfile.write(s)
    

def write_floats_to_memory(mapfile, floats, write_file):
    s = floats.tobytes()

    # write_file.write("floats sending back: ")
    # for i in floats:
    #     temp = "{0}, ".format(i)
    #     write_file.write(temp)
    # write_file.write("\n")

    write_to_memory(mapfile, s)


def write_all_zeroes(mapfile):
    mapfile.seek(4)

    s = ""
    for i in range(128 * 4):
        s += "\x00"
    mapfile.write(s)


def read_send_code(mapfile, write_file):
    mapfile.seek(0)

    code_str = mapfile.read(4) # 1 int, 4 bytes
    code_int = struct.unpack("<L", code_str)[0]

    return code_int


def read_ints_from_memory(mapfile, write_file):
    mapfile.seek(4)
    s = mapfile.read(128*4) # 128 ints 4 bytes each

    # https://stackoverflow.com/questions/8461798/how-can-i-struct-unpack-many-numbers-at-once
    inters = struct.unpack("<128L", s)

    # write_file.write("board ints recieved: ")
    # for i in inters:
    #     s = "{0}, ".format(i)
    #     write_file.write(s)
    # write_file.write("\n")
    return inters


def ensure_logging_directory():
    # get newest model directory
    log_dir_name = 'logs'
    log_dir = os.path.join(os.getcwd(), log_dir_name)
    
    # ensure data_dir exists
    if not os.path.isdir(log_dir):
        print('MODEL: making log directory at {0}', log_dir)
        try:
            os.mkdir(log_dir)
        except OSError as exc:
            pass


def serve_requests(memory, semaphore, mapfile, MODEL_NAME, write_file):
    send_code = 0

    with tf.Session() as sess:
        MODEL_PATH = os.path.join(os.getcwd(), 'data', MODEL_NAME) # probably should error check that data exists
        write_file.write('Using model at: {0}\n'.format(MODEL_PATH))

        saver = tf.train.import_meta_graph(os.path.join(MODEL_PATH, 'model.ckpt.meta'))
        graph = tf.get_default_graph()

        x_tensor = graph.get_tensor_by_name('x:0')
        train_bool = graph.get_tensor_by_name('train_bool:0')
        value_head_output = graph.get_tensor_by_name('value_head_output:0')
        policy_head_output = graph.get_tensor_by_name('policy_head_output/BiasAdd:0')

        saver.restore(sess, os.path.join(MODEL_PATH, 'model.ckpt'))
        
        requests_served = 0
        while True: # serving loop
            # s = "Request number: {0}\n".format(requests_served)
            # write_file.write(s)

            # write_file.write("Waiting to acquire the semaphore\n")
            semaphore.acquire()
            # write_file.write("Aquired the semaphore\n")


            # recieve info form c++
            inters = read_ints_from_memory(mapfile, write_file)
            
            


            ###### PROCESS DATA VIA FORWARD PASS
            x_data = [inters]
            res = sess.run([policy_head_output, value_head_output], feed_dict={x_tensor: x_data, train_bool: False})
            policy_calced = res[0][0]
            value_calced = res[1][0]
            together = np.append(policy_calced, value_calced)
            ######

            write_floats_to_memory(mapfile, together, write_file)

            send_code = 1
            write_send_code(mapfile, write_file)

            requests_served += 1

            while send_code == 1:
                # write_file.write("Releasing the semaphore\n")
                semaphore.release()

                # write_file.write("Waiting to acquire the semaphore\n")
                semaphore.acquire()

                send_code = read_send_code(mapfile, write_file)

            # write_file.write("Releasing the semaphore\n")
            semaphore.release()

            if send_code == 2:
                return requests_served


def main(SEMAPHORE_NAME, SHARED_MEMORY_NAME, MODEL_PATH):
    filename = "python_output_{0}_{1}.txt".format(SEMAPHORE_NAME, SHARED_MEMORY_NAME)
    ensure_logging_directory()
    file_path = os.path.join("logs", filename)
    write_file = open(file_path, 'w') # open file and overwrite
    
    s = "INITIALIZING PYTHON COMMUNICATOR\n"
    write_file.write(s)

    # write out init data
    s = "SEMAPHORE_NAME: {0}\n".format(SEMAPHORE_NAME)
    write_file.write(s)

    s = "SHARED_MEMORY_NAME: {0}\n".format(SHARED_MEMORY_NAME)
    write_file.write(s)

    s = "MODEL_PATH: {0}\n".format(MODEL_PATH)
    write_file.write(s)



    # semaphore opening and stuff
    memory = posix_ipc.SharedMemory(SHARED_MEMORY_NAME)
    semaphore = posix_ipc.Semaphore(SEMAPHORE_NAME)

    mapfile = mmap.mmap(memory.fd, memory.size) # MMap the shared memory

    # the mmapped has the file descriptor, it can be closed safely
    os.close(memory.fd)

    # the actual serving, and time it
    time_start = time.time()
    total_iterations = serve_requests(memory, semaphore, mapfile, MODEL_PATH, write_file)
    time_end = time.time()

    semaphore.close()
    mapfile.close()

    s = "{0} iterations complete in {1} seconds\n".format(total_iterations, time_end - time_start)
    write_file.write(s)
    write_file.close()
    time.sleep(8)


parser = argparse.ArgumentParser()
parser.add_argument("semaphore_name")
parser.add_argument("shared_memory_name")
parser.add_argument("model_name")
args = parser.parse_args()

SEMAPHORE_NAME = args.semaphore_name
SHARED_MEMORY_NAME = args.shared_memory_name
MODEL_NAME = args.model_name

main(SEMAPHORE_NAME, SHARED_MEMORY_NAME, MODEL_NAME)