#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <chrono>
#include <fstream>
#include <string>
#include <vector>

#include <iostream>
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>

#include <errno.h> 
#include <unistd.h> 
#include <time.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdarg.h>
#include <cstdio>
#include <inttypes.h>
#include <unistd.h>


struct new_params
{
    int size;
    std::string semaphore_name;
    std::string shared_memory_name;
    int permissions;
};

class PyCommunicator
{
    public:
        PyCommunicator(std::string name);
        int setup_python_communication();
        void clean_up();
        float* send_and_recieve(int32_t* arr_to_send);

    private:
        void call_python_script_helper();
        void send_end_code_python();
        int destroy_communication();
        std::string gen_random(const int len);
        int acquire_semaphore(sem_t *pSemaphore);
        int release_semaphore(sem_t *pSemaphore); 
        int send_and_recieve_helper(int32_t* arr_to_send);

        std::string model_name;

        sem_t *pSemaphore;
        void *pSharedMemory_code;
        void *pSharedMemory_rest;

        int fd;
        struct new_params params;


        int num_floats_recieve = 65;
        int num_ints_send = 128;
        float* float_arr_reciever;

        // sender flag
        int32_t send_code; // -1 is nothing, 0 is c sent, 1 is python sent
};

#endif