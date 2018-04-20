#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <semaphore.h>
#include <string.h>

#include "utils.h"

int release_semaphore(sem_t *pSemaphore) 
{
    int rc = 0;
    
    // printf("Releasing the semaphore.\n");
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

    // printf("Waiting to acquire the semaphore.\n");
    rc = sem_wait(pSemaphore);
    if(rc) 
    {
        printf("Acquiring the semaphore failed; errno is %d\n", errno);
    }

    return rc;
}