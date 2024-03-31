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

    if ((requestingWork = (bool *)malloc(nThreads * sizeof(bool))) == NULL)
    {
        perror("error on allocating memory for workers request");
        exit(EXIT_FAILURE);
    }

    if ((assignedWork = (bool *)malloc(nThreads * sizeof(bool))) == NULL)
    {
        perror("error on allocating memory for workers assigned");
        exit(EXIT_FAILURE);
    }

    if ((completedWork = (bool *)malloc(nThreads * sizeof(bool))) == NULL)
    {
        perror("error on allocating memory for workers completed");
        exit(EXIT_FAILURE);
    }

    int i;
    for (i = 0; i < nThreads; i++)
    {
        assignedWork[i] = false;
        requestingWork[i] = false;
        completedWork[i] = false;
        notFinishedWorkers[i] = true;
    }

    if ((workersWork = (SubArray *)malloc(nThreads * sizeof(SubArray))) == NULL)
    {
        perror("error on allocating memory for workers data");
        exit(EXIT_FAILURE);
    }

    pthread_cond_init(&workRequest, NULL);
    pthread_cond_init(&workCompleted, NULL);
    pthread_cond_init(&workAssigned, NULL);
}

void distributeSubArrays(SubArray *arraySub, int *array, int subSize, int fullSize)
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);

    if ((fullArray = (int *)malloc(fullSize * sizeof(int))) == NULL)
    {
        perror("error on allocating memory for full array");
        exit(EXIT_FAILURE);
    }

    if ((subArrays = (SubArray *)malloc(subSize * sizeof(SubArray))) == NULL)
    {
        perror("error on allocating memory for sub arrays");
        exit(EXIT_FAILURE);
    }

    memcpy(subArrays, arraySub, subSize * sizeof(SubArray));
    memcpy(fullArray, array, fullSize * sizeof(int));

    subArraySize = subSize;

    mutex_unlock();
}

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

int notFinished(int id)
{
    return notFinishedWorkers[id];
}

void requestWork(int id, SubArray *subArray)
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);

    waitingWork = true;
    requestingWork[id] = true;

    pthread_cond_signal(&workRequest);

    while (!assignedWork[id] && notFinishedWorkers[id])
    {
        if ((statusWor[id] = pthread_cond_wait(&workAssigned, &accessCR)) != 0)
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

void completeWork(int id)
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);
    
    completedWork[id] = true;

    waitingComplete = true;

    if ((statusWor[id] = pthread_cond_signal(&workCompleted)) != 0)
    {
        errno = statusWor[id];
        perror("error on signaling work completion");
        statusWor[id] = EXIT_FAILURE;
        pthread_exit(&statusWor[id]);
    }

    mutex_unlock();
}

void waitForWorkRequest()
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

    if ((statusDis = pthread_cond_broadcast(&workAssigned)) != 0)
    {
        errno = statusDis;
        perror("error on signaling work assignment");
        statusDis = EXIT_FAILURE;
        pthread_exit(&statusDis);
    }

    waitingWork = false;

    mutex_unlock();
}

void waitForWorkComplete()
{
    mutex_lock();
    pthread_once(&init, initializeSharedMemory);

    while (!waitingComplete)
    {
        if (pthread_cond_wait(&workCompleted, &accessCR) != 0)
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