#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <chrono>
#include <fstream>
#include <string>
#include <vector>
#include "communicator.hpp"

PyCommunicator::PyCommunicator(std::string name)
{
    model_name = name;

    // asigning values to struct
    params.semaphore_name = gen_random(10);
    params.shared_memory_name = gen_random(10);
    params.size = 4096;
    params.permissions = 0600;

    printf("params - size: %d\n", params.size);
    printf("params - semaphore_name: %s\n", params.semaphore_name.c_str());
    printf("params - shared_memory_name: %s\n", params.shared_memory_name.c_str());
    printf("params - permissions: %d\n", params.permissions);

    num_floats_recieve = 65;
    num_ints_send = 128;
    float_arr_reciever = (float*) malloc(num_floats_recieve * sizeof(float));

    call_python_script_helper();
}

int PyCommunicator::send_and_recieve_helper(int32_t* arr_to_send)
{
    send_code = 0;

    memcpy(pSharedMemory_code, &send_code, sizeof(int32_t));
    memcpy(pSharedMemory_rest, arr_to_send, num_ints_send * sizeof(int32_t));

    // printf("AS COLOR %i, I AM SENDING:\n", p_color);
    // for(int i = 0; i < 128; i++)
    // {
    //     printf("%i, ", int_arr_sender[i]);
    // }
    // printf("\n");
    // exit(0);


    // printf("Wrote send_code: %d\n", send_code);
    // printf("Wrote %d ints\n", num_ints_send);
    // printf("sizeof = %lu\n", sizeof(int32_t));
    // printf("length = %lu\n", num_ints_send * sizeof(int32_t));

    // Release the semaphore...
    int rc = release_semaphore(pSemaphore);
    // ...and wait for it to become available again. In real code 
    // I might want to sleep briefly before calling .acquire() in
    // order to politely give other processes an opportunity to grab
    // the semaphore while it is free so as to avoid starvation. But 
    // this code is meant to be a stress test that maximizes the 
    // opportunity for shared memory corruption and politeness is 
    // not helpful in stress tests.
    if (!rc)
    {
        rc = acquire_semaphore(pSemaphore);
    }
    if (rc) // aquiring failed
    {
        return 1;
    }
    else 
    {
        // I keep checking the shared memory until something new has 
        // been written.

        memcpy(&send_code, pSharedMemory_code, sizeof(int32_t));
        while ((!rc) && send_code == 0) 
        {            
            rc = release_semaphore(pSemaphore);
            if (!rc) 
            {
                rc = acquire_semaphore(pSemaphore);
            }
            memcpy(&send_code, pSharedMemory_code, sizeof(int32_t));
        }

        if (rc)  // aquiring failed
        {
            return 1;
        }

        // send_code is not 0, means we have recieved data
        // first 64 are policies, last is value prediction
        memcpy(float_arr_reciever, pSharedMemory_rest, num_floats_recieve * sizeof(float));

        // for(int i = 0; i < 64; i++)
        // {
        //     printf("%f,", float_arr_reciever[i]);
        // }
        // printf("\n");
    }

    return 0;
}

float* PyCommunicator::send_and_recieve(int32_t* arr_to_send)
{
    int ret = send_and_recieve_helper(arr_to_send);

    if(ret != 0)
    {
        printf("Problem with send_and_recieve in PyCommunicator return code is %i, exiting...\n", ret);
        exit(0);
    }

    return float_arr_reciever;
}


void PyCommunicator::call_python_script_helper()
{
    std::string command = "python python_model_communicator.py " + params.semaphore_name + " " + 
                            params.shared_memory_name + " " + 
                            model_name;

    printf("EXECUTING: %s\n", command.c_str());
    pid_t pid = fork();
    if(pid != 0)
    {
        int ret_val = system(command.c_str());
        exit(0);
    }
}


// model and python communication
// 0 is success
// 1 is semaphore error
// 2 is shared memory error
int PyCommunicator::setup_python_communication()
{
    // printf("SEEDING RANDOM\n");
    // srand (time(NULL));

    pSemaphore = NULL;
    pSharedMemory_code = NULL;
    pSharedMemory_rest = NULL;

    // sender flag
    send_code = -1; // -1 is uninitilized, 0 is c sent, 1 is python sent, 2 is c sent python kill

    
    printf("COMMUNICATION CHANNEL INITIALIZING IN C++!\n");

    // Create the shared memory
    fd = shm_open(params.shared_memory_name.c_str(), O_RDWR | O_CREAT | O_EXCL, params.permissions);    

    if (fd == -1) 
    {
        fd = 0;
        printf("Creating the shared memory failed\n");
        return 2;
    }

    else 
    {
        // The memory is created as a file that's 0 bytes long. Resize it.
        int rc = ftruncate(fd, params.size);
        if (rc) 
        {
            printf("Resizing the shared memory failed\n");
            return 2;
        }
        else 
        {
            // MMap the shared memory
            //void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
            pSharedMemory_code = mmap((void *)0, (size_t)params.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (pSharedMemory_code == MAP_FAILED) 
            {
                pSharedMemory_code = NULL;
                printf("MMapping the shared memory failed\n");
                return 2;
            }
            else 
            {
                pSharedMemory_rest = (void*) (((int32_t*) pSharedMemory_code) + 1);
                printf("pSharedMemory_code = %p\n", pSharedMemory_code);
                printf("pSharedMemory_rest = %p\n", pSharedMemory_rest);
            }
        }
    }
    
    if (pSharedMemory_code) 
    {
        // Create the semaphore
        pSemaphore = sem_open(params.semaphore_name.c_str(), O_CREAT, params.permissions, 0);
    
        if (pSemaphore == SEM_FAILED) 
        {
            printf("Creating the semaphore failed\n");
            return 1;
        }
        else 
        {
            printf("pSemaphore =  %p\n", (void *)pSemaphore);
        }
    }

    printf("communication channel established\n");
    return 0;
}

void PyCommunicator::send_end_code_python()
{
    send_code = 2;
    memcpy(pSharedMemory_code, &send_code, sizeof(int32_t));
}

// 0 is success
// 1 is semaphore error
// 2 is shared memory error
int PyCommunicator::destroy_communication()
{
    send_end_code_python();

    // Announce for one last time that the semaphore is free again so that python can quit
    printf("Final release of the semaphore and send_code followed by a 5 second pause\n"); 
    int rc = release_semaphore(pSemaphore);
    sleep(1); // race condition, where the python takes 5 seconds to quit

    printf("Final wait to acquire the semaphore\n"); 
    rc = acquire_semaphore(pSemaphore);
    if (!rc) 
    {
        printf("Destroying the shared memory.\n");

        rc = munmap(pSharedMemory_code, (size_t)params.size); // Un mmap the memory
        if (rc) 
        {
            printf("Unmapping the memory failed\n");
            return 2;
        }
        
        if (-1 == close(fd)) // close file descriptor 
        {
            printf("Closing the memory's file descriptor failed\n");
            return 2;
        }
    
        rc = shm_unlink(params.shared_memory_name.c_str()); // destroy the shared memory.
        if (rc) 
        {
            printf("Unlinking the memory failed\n");
            return 2;
        }
    }

    printf("Destroying the semaphore.\n");
    // Clean up the semaphore
    rc = sem_close(pSemaphore);
    if (rc) 
    {
        printf("Closing the semaphore failed\n");
        return 1;
        
    }
    rc = sem_unlink(params.semaphore_name.c_str());
    if (rc) 
    {
        printf("Unlinking the semaphore failed\n");
        return 1;
        
    }

    return 0;
}

void PyCommunicator::clean_up()
{
    printf("begining communication cleanup\n");
    int ret_val = destroy_communication();
    if(ret_val == 1)
    {
        printf("Setup python communication returned with error 1 (problems with semaphore) exiting.");
        exit(0);
    }
    else if(ret_val == 2)
    {
        printf("Setup python communication returned with error 2 (problems with shared memory) exiting.");
        exit(0);
    }
    free(float_arr_reciever);
    printf("finished communication cleanup\n");
}


// https://stackoverflow.com/questions/440133/how-do-i-create-a-random-alpha-numeric-string-in-c
std::string PyCommunicator::gen_random(const int len) 
{
    std::string new_str;
    for(int i = 0; i < len; i++)
    {
        new_str.append(" ");
    }

    static const char alphanum[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) 
    {
        new_str[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    return new_str;
}

int PyCommunicator::release_semaphore(sem_t *pSemaphore) 
{
    int rc = 0;
    rc = sem_post(pSemaphore);
    if(rc) 
    {
        printf("Releasing the semaphore failed\n");
    }
    return rc;
}

int PyCommunicator::acquire_semaphore(sem_t *pSemaphore) 
{
    int rc = 0;
    rc = sem_wait(pSemaphore);
    if(rc) 
    {
        printf("Acquiring the semaphore failed\n");
    }
    return rc;
}