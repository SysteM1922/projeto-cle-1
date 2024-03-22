#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include "structs.h"

char *fileName;

static pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;

static pthread_once_t init = PTHREAD_ONCE_INIT;

static pthread_cond_t workRequest = PTHREAD_COND_INITIALIZER;

static pthread_cond_t workCompleted = PTHREAD_COND_INITIALIZER;

static pthread_cond_t workAssigned = PTHREAD_COND_INITIALIZER;

static int subArraySize;

static bool waitingWork;

static int statusMon;

static bool *notFinishedWorkers;

static bool *requestingWork;

static bool *assignedWork;

static int *fullArray;

extern int statusDis;

extern int *statusWor;

extern int nThreads;

static SubArray *workersWork;

void mutex_lock()
{
    if ((statusMon = pthread_mutex_lock(&accessCR)) != 0)
    {
        errno = statusMon;
        perror("error on entering monitor(CF)");
        statusMon = EXIT_FAILURE;
        pthread_exit(&statusMon);
    }
}

void mutex_unlock()
{
    if ((statusMon = pthread_mutex_unlock(&accessCR)) != 0)
    {
        errno = statusMon;
        perror("error on exiting monitor(CF)");
        statusMon = EXIT_FAILURE;
        pthread_exit(&statusMon);
    }
}

void initializeSharedMemory()
{
    if ((notFinishedWorkers = (bool *)malloc(nThreads * sizeof(bool))) == NULL)
    {
        perror("error on allocating memory for workers status");
        exit(EXIT_FAILURE);
    }

    int i;
    for (i = 0; i < nThreads; i++)
    {
        notFinishedWorkers[i] = true;
    }

    if ((requestingWork = (bool *)malloc(nThreads * sizeof(bool))) == NULL)
    {
        perror("error on allocating memory for workers request");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < nThreads; i++)
    {
        requestingWork[i] = false;
    }

    if ((assignedWork = (bool *)malloc(nThreads * sizeof(bool))) == NULL)
    {
        perror("error on allocating memory for workers assigned");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < nThreads; i++)
    {
        assignedWork[i] = false;
    }

    if ((workersWork = (SubArray *)malloc((nThreads * 2 - 1) * sizeof(SubArray))) == NULL)
    {
        perror("error on allocating memory for workers data");
        exit(EXIT_FAILURE);
    }

    pthread_cond_init(&workRequest, NULL);
    pthread_cond_init(&workCompleted, NULL);
    pthread_cond_init(&workAssigned, NULL);
}

void distributeSubArrays(SubArray *arraySub, int *array, int size)
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);

    memcpy(workersWork, arraySub, size * sizeof(SubArray));
    memcpy(fullArray, array, size * sizeof(int));
    subArraySize = size;

    mutex_unlock();
}

bool sortCompleted()
{
    return workersWork[subArraySize - 1].completed;
}

int notFinished(int id)
{
    return notFinishedWorkers[id];
}

void requestWork(int id, SubArray *subArray)
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);

    waitingWork = true;
    requestingWork[id] = 1;

    pthread_cond_signal(&workRequest);

    while (!assignedWork[id] && notFinishedWorkers[id])
    {
        if ((statusMon = pthread_cond_wait(&workAssigned, &accessCR)) != 0)
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
    }

    mutex_unlock();
}

SubArray getWorkerWork(int id)
{
    int flag = -1;
    int i;
    for (i = subArraySize - 1; i > -1; i--)
    {
        if (workersWork[i].threadId == id && !workersWork[i].completed)
        {
            flag = -2;
            if (workersWork[i].dependent_1 == -1 || (workersWork[workersWork[i].dependent_1].completed && workersWork[workersWork[i].dependent_2].completed))
            {
                return workersWork[i];
            }
        }
    }

    SubArray subArray;
    subArray.threadId = flag;

    return subArray;
}

void assignWork()
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);

    SubArray subArray;

    int i;
    for (i = 0; i < nThreads; i++)
    {
        if (requestingWork[i])
        {
            subArray = getWorkerWork(i);
            if (subArray.threadId == -2)
            {
                continue;
            }
            else if (subArray.threadId == -1)
            {
                notFinishedWorkers[i] = false;
                continue;
            }
            workersWork[i] = subArray;
            assignedWork[i] = true;
            requestingWork[i] = false;
        }
    }
    
    pthread_cond_signal(&workAssigned);
    waitingWork = false;

    mutex_unlock();
}

void completeWork(int id)
{
    int i;
    for (i = subArraySize - 1; i > -1; i--)
    {
        if (workersWork[i].threadId == id && workersWork[i].completed == false)
        {
            workersWork[i].completed = true;
            break;
        }
    }
}

void waitForWorkers()
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);

    while (!waitingWork)
    {
        if (pthread_cond_wait(&workRequest, &accessCR) != 0)
        {
            perror("error on waiting for work request");
            statusMon = EXIT_FAILURE;
            pthread_exit(&statusMon);
        }
    }

    int i;
    for (i = 0; i < nThreads; i++)
    {
        if (requestingWork[i])
        {
            completeWork(i);
        }
    }

    mutex_unlock();
}

void putFileName(char *file)
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);
    
    fileName = file;

    mutex_unlock();
}

void getFileName(char **file)
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);

    *file = fileName;

    mutex_unlock();
}

void putArray(int *array, int start, int size)
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);

    memcpy(fullArray + start, array, size * sizeof(int));
    free(array);

    mutex_unlock();
}

void getArray(int **array, int start, int size)
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);

    *array = malloc(size * sizeof(int));
    memcpy(*array, fullArray + start, size * sizeof(int));

    mutex_unlock();
}

void getFullArray(int **array)
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);

    *array = fullArray;

    mutex_unlock();
}