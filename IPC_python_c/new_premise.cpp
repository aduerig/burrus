#include <stdio.h> 
#include <errno.h> 
#include <unistd.h> 
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdarg.h>
#include <cstdio>
#include <stdlib.h>
#include <string>
#include <inttypes.h>
#include <unistd.h>

struct new_params
{
    int size;
    std::string semaphore_name;
    std::string shared_memory_name;
    int permissions;
    int iterations;
};


// largely taken from /demo/ of http://semanchuk.com/philip/posix_ipc/

void call_python_script_helper(new_params params);
std::string get_current_time();
void fill_random_ints(int* ints_to_fill, int num_ints);
std::string gen_random(const int len);
int acquire_semaphore(sem_t *);
int release_semaphore(sem_t *);


int main()
{ 
    printf("SEEDING RANDOM\n");
    srand (time(NULL));

    sem_t *pSemaphore = NULL;
    int rc;
    std::string s;
    void *pSharedMemory_code = NULL;
    void *pSharedMemory_rest = NULL;
    int curr_iter = 0;
    int done = 0;
    int fd;
    struct new_params params;

    // sender flag
    int32_t send_code = -1; // -1 is nothing, 0 is c sent, 1 is python sent

    // data holders
    int num_ints = 128;
    int num_floats = 64;

    int32_t* dummy_int_arr = (int32_t*) malloc(num_ints * sizeof(int32_t));
    int32_t* last_dummy_arr_sent = (int32_t*) malloc(num_ints * sizeof(int32_t));

    float* float_reciever = (float*) malloc(num_floats * sizeof(float));

    
    printf("SENDER INITIALIZED C++!\n");
    
    // asigning values to struct
    params.size = 4096;
    params.semaphore_name = gen_random(10);
    params.shared_memory_name = gen_random(10);
    params.permissions = 0600;
    params.iterations = 500000;

    printf("params - size: %d\n", params.size);
    printf("params - semaphore_name: %s\n", params.semaphore_name.c_str());
    printf("params - shared_memory_name: %s\n", params.shared_memory_name.c_str());
    printf("params - permissions: %d\n", params.permissions);
    printf("params - iterations: %d\n", params.iterations);

    // Create the shared memory
    fd = shm_open(params.shared_memory_name.c_str(), O_RDWR | O_CREAT | O_EXCL, params.permissions);    

    // this code is magic, dont change
    if (fd == -1) 
    {
        fd = 0;
        printf("Creating the shared memory failed; errno is %d\n", errno);
    }

    else 
    {
        // The memory is created as a file that's 0 bytes long. Resize it.
        rc = ftruncate(fd, params.size);
        if (rc) 
        {
            printf("Resizing the shared memory failed; errno is %d\n", errno);
        }
        else 
        {
            // MMap the shared memory
            //void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
            pSharedMemory_code = mmap((void *)0, (size_t)params.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (pSharedMemory_code == MAP_FAILED) 
            {
                pSharedMemory_code = NULL;
                printf("MMapping the shared memory failed; errno is %d\n", errno);
            }
            else 
            {
                pSharedMemory_rest = (void*) (((int32_t*) pSharedMemory_code) + 1);
                printf("pSharedMemory_code = %p\n", pSharedMemory_code);
                printf("pSharedMemory_rest = %p\n", pSharedMemory_rest);
            }
        }
    }
    // end magic
    
    if (pSharedMemory_code) 
    {
        // Create the semaphore
        pSemaphore = sem_open(params.semaphore_name.c_str(), O_CREAT, params.permissions, 0);
    
        if (pSemaphore == SEM_FAILED) 
        {
            printf("Creating the semaphore failed; errno is %d\n", errno);
            
        }
        else 
        {
            printf("the semaphore is %p\n", (void *)pSemaphore);
            
            call_python_script_helper(params);

            curr_iter = 0;
            while (!done && curr_iter < params.iterations)
            {
                // printf("iteration %d\n", curr_iter);
                    
                fill_random_ints(dummy_int_arr, num_ints);

                // memcpy = (dest, source, num bytes)
                send_code = 0;
                memcpy(pSharedMemory_code, &send_code, sizeof(int32_t));
                memcpy(pSharedMemory_rest, dummy_int_arr, num_ints * sizeof(int32_t));
                memcpy(last_dummy_arr_sent, dummy_int_arr, num_ints * sizeof(int32_t));

                // printf("Wrote send_code: %d\n", send_code);
                // printf("Wrote %d ints\n", num_ints);
                // printf("sizeof = %lu\n", sizeof(int32_t));
                // printf("length = %lu\n", num_ints * sizeof(int32_t));

                // Release the semaphore...
                rc = release_semaphore(pSemaphore);
                // ...and wait for it to become available again. In real code 
                // I might want to sleep briefly before calling .acquire() in
                // order to politely give other processes an opportunity to grab
                // the semaphore while it is free so as to avoid starvation. But 
                // this code is meant to be a stress test that maximizes the 
                // opportunity for shared memory corruption and politeness is 
                // not helpful in stress tests.
                if (!rc)
                {
                    // printf("blocking aquire\n");
                    rc = acquire_semaphore(pSemaphore);
                }

                if (rc) // aquiring failed
                {
                    done = 1;
                }
                
                else 
                {
                    // I keep checking the shared memory until something new has 
                    // been written.

                    memcpy(&send_code, pSharedMemory_code, sizeof(int32_t));
                    while ((!rc) && send_code == 0) 
                    {
                        // Nothing new; give Mrs. Conclusion another chance to respond.
                        // printf("Read %zu characters '%s'\n", strlen((char *)pSharedMemory_rest), (char *)pSharedMemory_rest);
                        

                        rc = release_semaphore(pSemaphore);
                        if (!rc) 
                        {
                            rc = acquire_semaphore(pSemaphore);
                        }
                        memcpy(&send_code, pSharedMemory_code, sizeof(int32_t));
                    }

                    if (rc) 
                    {
                        done = 1;
                    }

                    // send_code is not 0, means we have recieved data
                    memcpy(float_reciever, pSharedMemory_rest, num_floats * sizeof(float));

                    // for(int i = 0; i < 64; i++)
                    // {
                    //     printf("%f,", float_reciever[i]);
                    // }
                    // printf("\n");
                }
                
                curr_iter++;
            }

            send_code = 2;
            memcpy(pSharedMemory_code, &send_code, sizeof(int32_t));

            // Announce for one last time that the semaphore is free again so that python can quit

            printf("Final release of the semaphore and send_code followed by a 5 second pause\n"); 
            rc = release_semaphore(pSemaphore);
            sleep(5); // race condition, where the python takes 5 seconds to quit

            printf("Final wait to acquire the semaphore\n"); 
            rc = acquire_semaphore(pSemaphore);
            if (!rc) 
            {
                printf("Destroying the shared memory.\n");

                rc = munmap(pSharedMemory_code, (size_t)params.size); // Un mmap the memory
                if (rc) 
                {
                    printf("Unmapping the memory failed; errno is %d\n", errno);
                }
                
                if (-1 == close(fd)) // close file descriptor 
                {
                    printf("Closing the memory's file descriptor failed; errno is %d\n", errno);
                }
            
                rc = shm_unlink(params.shared_memory_name.c_str()); // destroy the shared memory.
                if (rc) 
                {
                    printf("Unlinking the memory failed; errno is %d\n", errno);
                }
            }
        }

        printf("Destroying the semaphore.\n");
        // Clean up the semaphore
        rc = sem_close(pSemaphore);
        if (rc) 
        {
            printf("Closing the semaphore failed; errno is %d\n", errno);
            
        }
        rc = sem_unlink(params.semaphore_name.c_str());
        if (rc) 
        {
            printf("Unlinking the semaphore failed; errno is %d\n", errno);
            
        }
    }

    free(dummy_int_arr);
    free(last_dummy_arr_sent);
    free(float_reciever);

    printf("c++ script has finished\n");
    return 0; 
}

void call_python_script_helper(new_params params)
{
    std::string command = "python conclusion.py " + params.semaphore_name + " " + 
                            params.shared_memory_name + " ";
    pid_t pid = fork();
    if(pid != 0)
    {
        system(command.c_str());
        exit(0);
    }
}


std::string get_current_time() 
{
    time_t the_time;
    struct tm *the_localtime;

    the_time = time(NULL);
    the_localtime = localtime(&the_time);
    return asctime(the_localtime);
}

void fill_random_ints(int* ints_to_fill, int num_ints)
{
    for(int i = 0; i < num_ints; i++)
    {
        // ints_to_fill[i] = rand();
        ints_to_fill[i] = i;
    }
}

// https://stackoverflow.com/questions/440133/how-do-i-create-a-random-alpha-numeric-string-in-c
std::string gen_random(const int len) 
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

int release_semaphore(sem_t *pSemaphore) 
{
    int rc = 0;
    rc = sem_post(pSemaphore);
    if(rc) 
    {
        printf("Releasing the semaphore failed; errno is %d\n", errno);
    }
    return rc;
}

int acquire_semaphore(sem_t *pSemaphore) 
{
    int rc = 0;
    rc = sem_wait(pSemaphore);
    if(rc) 
    {
        printf("Acquiring the semaphore failed; errno is %d\n", errno);
    }
    return rc;
}