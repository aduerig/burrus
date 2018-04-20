# Python modules
import mmap
import os
import sys
import hashlib
import struct
import time
import numpy as np
import argparse

# 3rd party modules
import posix_ipc

# Utils
import utils


def serve_requests(memory, semaphore, mapfile, write_file):
    curr_iter = 0
    send_code = 0
    set_floaters = np.arange(1, 65, dtype=np.float32)
    
    while True:
        # s = "on iteration: {0}\n".format(i)
        # write_file.write(s)

        # write_file.write("Waiting to acquire the semaphore\n")
        semaphore.acquire()
        # write_file.write("Aquired the semaphore\n")


        # recieve info
        inters = utils.read_ints_from_memory(mapfile, write_file)
        
        # process info here

        # get floats to send back
        # floats_to_send = utils.get_floats()
        floats_to_send = set_floaters
        utils.write_floats_to_memory(mapfile, floats_to_send)
        # utils.write_all_zeroes(mapfile)

        send_code = 1
        utils.write_send_code(mapfile, write_file)

        curr_iter += 1

        while send_code == 1:
            # write_file.write("Releasing the semaphore\n")
            semaphore.release()

            # write_file.write("Waiting to acquire the semaphore\n")
            semaphore.acquire()

            send_code = utils.read_send_code(mapfile, write_file)

        # write_file.write("Releasing the semaphore\n")
        semaphore.release()

        if send_code == 2:
            return curr_iter


def main(SEMAPHORE_NAME, SHARED_MEMORY_NAME):
    # open file and overwrite
    write_file = open("python_out.txt", 'w')
    
    s = "INITIALIZING PYTHON\n"
    write_file.write(s)

    # write out init data
    s = "SEMAPHORE_NAME: {0}\n".format(SEMAPHORE_NAME)
    write_file.write(s)

    s = "SHARED_MEMORY_NAME: {0}\n".format(SHARED_MEMORY_NAME)
    write_file.write(s)



    # semaphore opening and stuff
    memory = posix_ipc.SharedMemory(SHARED_MEMORY_NAME)
    semaphore = posix_ipc.Semaphore(SEMAPHORE_NAME)

    mapfile = mmap.mmap(memory.fd, memory.size) # MMap the shared memory

    # Once I've mmapped the file descriptor, I can close it without
    # interfering with the mmap. This also demonstrates that os.close() is a
    # perfectly legitimate alternative to the SharedMemory's close_fd() method.
    os.close(memory.fd)


    # the actual serving, and time it
    time_start = time.time()
    total_iterations = serve_requests(memory, semaphore, mapfile, write_file)
    time_end = time.time()

    semaphore.close()
    mapfile.close()

    s = "{0} iterations complete in {1} seconds\n".format(total_iterations, time_end - time_start)
    write_file.write(s)
    write_file.close()


# PY_MAJOR_VERSION = sys.version_info[0]

parser = argparse.ArgumentParser()
parser.add_argument("semaphore_name")
parser.add_argument("shared_memory_name")
args = parser.parse_args()

SEMAPHORE_NAME = args.semaphore_name
SHARED_MEMORY_NAME = args.shared_memory_name


main(SEMAPHORE_NAME, SHARED_MEMORY_NAME)