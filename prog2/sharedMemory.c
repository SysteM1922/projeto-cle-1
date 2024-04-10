/**
 * \file sharedMemory.c
 * 
 * \brief Shared Memory module.
 * 
 * This module provides the shared memory functions implementation.
 * 
 * \author Guilherme Antunes - 103600
 * \author Pedro Rasinhas - 103541
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include "structs.h"

/** \brief file name */
char *fileName;

/** \brief access control */
static pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;

/** \brief initialization control */
static pthread_once_t init = PTHREAD_ONCE_INIT;

/** \brief work request condition */
static pthread_cond_t workRequest = PTHREAD_COND_INITIALIZER;

/** \brief work completed condition */
static pthread_cond_t workCompleted = PTHREAD_COND_INITIALIZER;

/** \brief work assigned condition */
static pthread_cond_t workAssigned = PTHREAD_COND_INITIALIZER;

/** \brief sub array size */
static int subArraySize;

/** \brief waiting work flag */
static bool waitingWork;

/** \brief waiting complete flag */
static bool waitingComplete;

/** \brief completed work flags */
static bool *completedWork;

/** \brief not finished workers flags */
static bool *notFinishedWorkers;

/** \brief requesting work flags */
static bool *requestingWork;

/** \brief assigned work flags */
static bool *assignedWork;

/** \brief full array */
static int *fullArray;

/** \brief status monitor */
static int statusMon;

/** \brief status distributor */
extern int statusDis;

/** \brief status workers */
extern int *statusWor;

/** \brief number of threads */
extern int nThreads;

/** \brief workers work */
static SubArray *workersWork;

/** \brief sub arrays */
static SubArray *subArrays;

/**
 * \brief Function mutex_lock.
 * 
 * Locks the access to the shared memory.
 */
void mutex_lock()
{
    if ((statusMon = pthread_mutex_lock(&accessCR)) != 0)       /* lock access to shared memory */
    {
        errno = statusMon;
        perror("error on entering monitor(CF)");
        statusMon = EXIT_FAILURE;
        pthread_exit(&statusMon);
    }
}

/**
 * \brief Function mutex_unlock.
 * 
 * Unlocks the access to the shared memory.
 */
void mutex_unlock()
{
    if ((statusMon = pthread_mutex_unlock(&accessCR)) != 0)     /* unlock access to shared memory */
    {
        errno = statusMon;
        perror("error on exiting monitor(CF)");
        statusMon = EXIT_FAILURE;
        pthread_exit(&statusMon);
    }
}


/**
 * \brief Function initializeSharedMemory.
 * 
 * Initializes the shared memory.
 */
void initializeSharedMemory()
{
    if ((notFinishedWorkers = (bool *)malloc(nThreads * sizeof(bool))) == NULL)     /* allocate memory for workers status */
    {
        perror("error on allocating memory for workers status");
        exit(EXIT_FAILURE);
    }

    if ((requestingWork = (bool *)malloc(nThreads * sizeof(bool))) == NULL)         /* allocate memory for workers request */
    {
        perror("error on allocating memory for workers request");
        exit(EXIT_FAILURE);
    }

    if ((assignedWork = (bool *)malloc(nThreads * sizeof(bool))) == NULL)           /* allocate memory for workers assigned */
    {
        perror("error on allocating memory for workers assigned");
        exit(EXIT_FAILURE);
    }

    if ((completedWork = (bool *)malloc(nThreads * sizeof(bool))) == NULL)          /* allocate memory for workers completed */
    {
        perror("error on allocating memory for workers completed");
        exit(EXIT_FAILURE);
    }

    int i;
    for (i = 0; i < nThreads; i++)          /* initialize workers flags */
    {
        assignedWork[i] = false;
        requestingWork[i] = false;
        completedWork[i] = false;
        notFinishedWorkers[i] = true;
    }

    if ((workersWork = (SubArray *)malloc(nThreads * sizeof(SubArray))) == NULL)        /* allocate memory for workers data */
    {
        perror("error on allocating memory for workers data");
        exit(EXIT_FAILURE);
    }

    pthread_cond_init(&workRequest, NULL);      /* initialize work request condition */
    pthread_cond_init(&workCompleted, NULL);    /* initialize work completed condition */
    pthread_cond_init(&workAssigned, NULL);     /* initialize work assigned condition */
}

/**
 * \brief Function distributeSubArrays.
 * 
 * Put the sub arrays in the shared memory.
 * 
 * \param arraySub sub arrays
 * \param array full array
 * \param subSize sub array size
 * \param fullSize full array size
 */
void distributeSubArrays(SubArray *arraySub, int *array, int subSize, int fullSize)
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);

    if ((fullArray = (int *)malloc(fullSize * sizeof(int))) == NULL)            /* allocate memory for full array */
    {
        perror("error on allocating memory for full array");
        exit(EXIT_FAILURE);
    }

    if ((subArrays = (SubArray *)malloc(subSize * sizeof(SubArray))) == NULL)   /* allocate memory for sub arrays */
    {
        perror("error on allocating memory for sub arrays");
        exit(EXIT_FAILURE);
    }

    memcpy(subArrays, arraySub, subSize * sizeof(SubArray));            /* copy sub arrays to shared memory */
    memcpy(fullArray, array, fullSize * sizeof(int));                   /* copy full array to shared memory */

    subArraySize = subSize;

    mutex_unlock();
}

/**
 * \brief Function sortCompleted.
 * 
 * Checks if the sort is completed.
 * 
 * \return true if the sort is completed, false otherwise
 */
bool sortCompleted()
{
    if (subArrays[0].completed)
    {
        for (int i = 0; i < nThreads; i++)
        {
            notFinishedWorkers[i] = false;
        }
        pthread_cond_signal(&workAssigned);
        return true;
    }
    return false;
}

/**
 * \brief Function notFinished.
 * 
 * Checks if the worker is not finished.
 * 
 * \param id worker id
 * \return true if the worker is not finished, false otherwise
 */
int notFinished(int id)
{
    return notFinishedWorkers[id];
}

/**
 * \brief Function requestWork.
 * 
 * Requests work to the distributor.
 * 
 * \param id worker id
 * \param subArray sub array
 */
void requestWork(int id, SubArray *subArray)
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);

    waitingWork = true;
    requestingWork[id] = true;

    pthread_cond_signal(&workRequest);

    while (!assignedWork[id] && notFinishedWorkers[id])
    {
        if ((statusWor[id] = pthread_cond_wait(&workAssigned, &accessCR)) != 0)     /* wait for work assignment */
        {
            errno = statusMon;
            perror("error on waiting for work assignment");
            statusMon = EXIT_FAILURE;
            pthread_exit(&statusMon);
        }
    }

    if (notFinishedWorkers[id])     
    {
        *subArray = workersWork[id];
        assignedWork[id] = false;
    }

    mutex_unlock();
}

/**
 * \brief Function getWorkerWork.
 * 
 * Gets the worker work.
 * 
 * \param id worker id
 * \return sub array
 */
SubArray getWorkerWork(int id)
{
    int flag = -1;
    int i;
    for (i = subArraySize - 1; i > -1; i--)
    {
        if (subArrays[i].threadId == id && !subArrays[i].completed)
        {
            flag = -2;
            if (subArrays[i].dependent_1 == -1 || (subArrays[subArrays[i].dependent_1].completed && subArrays[subArrays[i].dependent_2].completed))
            {
                return subArrays[i];
            }
        }
    }

    SubArray subArray;
    subArray.threadId = flag;

    return subArray;
}

/**
 * \brief Function completeWork.
 * 
 * Completes the work.
 * 
 * \param id worker id
 */
void completeWork(int id)
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);
    
    completedWork[id] = true;

    waitingComplete = true;

    if ((statusWor[id] = pthread_cond_signal(&workCompleted)) != 0)     /* signal work completion */
    {
        errno = statusWor[id];
        perror("error on signaling work completion");
        statusWor[id] = EXIT_FAILURE;
        pthread_exit(&statusWor[id]);
    }

    mutex_unlock();
}

/**
 * \brief Function waitForWorkRequest.
 * 
 * Distributor waits for work request.
 */
void waitForWorkRequest()
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);

    while (!waitingWork)
    {
        if (pthread_cond_wait(&workRequest, &accessCR) != 0)        /* wait for work request */
        {
            perror("error on waiting for work request");
            statusMon = EXIT_FAILURE;
            pthread_exit(&statusMon);
        }
    }

    SubArray subArray;

    int i;
    for (i = 0; i < nThreads; i++)
    {
        if (requestingWork[i])
        {
            subArray = getWorkerWork(i);

            if (subArray.threadId == -1)
            {
                notFinishedWorkers[i] = false;
                continue;
            }
            else if (subArray.threadId == -2)
            {
                continue;
            }
            else
            {
                workersWork[i] = subArray;
                requestingWork[i] = false;
                assignedWork[i] = true;
            }
        }
    }

    if ((statusDis = pthread_cond_broadcast(&workAssigned)) != 0)       /* signal work assignment */
    {
        errno = statusDis;
        perror("error on signaling work assignment");
        statusDis = EXIT_FAILURE;
        pthread_exit(&statusDis);
    }

    waitingWork = false;

    mutex_unlock();
}

/**
 * \brief Function waitForWorkComplete.
 * 
 * Distributor waits for work completion.
 */
void waitForWorkComplete()
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);

    while (!waitingComplete)
    {
        if (pthread_cond_wait(&workCompleted, &accessCR) != 0)      /* wait for work completion */
        {
            perror("error on waiting for work completion");
            statusMon = EXIT_FAILURE;
            pthread_exit(&statusMon);
        }
    }

    int i;
    for (i = 0; i < nThreads; i++)
    {
        if (completedWork[i])
        {
            workersWork[i].completed = true;
            subArrays[workersWork[i].id] = workersWork[i];
            completedWork[i] = false;
        }
    }

    waitingComplete = false;

    mutex_unlock();
}

/**
 * \brief Function putFileName.
 * 
 * Puts the file name in the shared memory.
 * 
 * \param file file name
 */
void putFileName(char *file)
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);

    fileName = file;

    mutex_unlock();
}

/**
 * \brief Function getFileName.
 * 
 * Gets the file name from the shared memory.
 * 
 * \param file file name
 */
void getFileName(char **file)
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);

    *file = fileName;

    mutex_unlock();
}

/**
 * \brief Function putArray.
 * 
 * Puts the array in the shared memory.
 * 
 * \param array array
 * \param start start index
 * \param size array size
 */
void putArray(int *array, int start, int size)
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);

    memcpy(fullArray + start, array, size * sizeof(int));       /* copy array to shared memory */
    free(array);

    mutex_unlock();
}

/**
 * \brief Function getArray.
 * 
 * Gets the array from the shared memory.
 * 
 * \param array array
 * \param start start index
 * \param size array size
 */
void getArray(int **array, int start, int size)
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);

    *array = malloc(size * sizeof(int));
    memcpy(*array, fullArray + start, size * sizeof(int));      /* copy array from shared memory */

    mutex_unlock();
}

/**
 * \brief Function getFullArray.
 * 
 * Gets the full array from the shared memory.
 * 
 * \param array full array
 */
void getFullArray(int **array)
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);

    *array = fullArray;

    mutex_unlock();
}