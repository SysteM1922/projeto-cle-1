#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <bits/time.h>

#include "sharedMemory.h"
#include "distributor.h"
#include "worker.h"
#include "bitonicSort.h"

int sortType = 0;
int fullSize;
int nThreads;

int statusDis;
int *statusWor;

int *workerId;

pthread_t tIdDis;
pthread_t *tIdWor;

DistributorArgs distributor_args;

/**
 * \brief Get the process time that has elapsed since last call of this time.
 *
 * \return process elapsed time
 */
static double get_delta_time(void)
{
    static struct timespec t0, t1;
    t0 = t1;
    if (clock_gettime(CLOCK_MONOTONIC, &t1) != 0)
    {
        perror("clock_gettime");
        exit(1);
    }
    return (double)(t1.tv_sec - t0.tv_sec) +
           1.0e-9 * (double)(t1.tv_nsec - t0.tv_nsec);
}

void initializeDistributor()
{
    distributor_args.sortType = sortType;

    if (pthread_create(&tIdDis, NULL, distributor, &distributor_args) != 0)
    {
        perror("error on creating thread distributor");
        exit(EXIT_FAILURE);
    }
}

void initializeWorkers()
{
    int i;

    for (i = 0; i < nThreads; i++)
    {
        if (pthread_create(&tIdWor[i], NULL, worker, &workerId[i]) != 0)
        {
            perror("error on creating thread worker");
            exit(EXIT_FAILURE);
        }
    }
}

void waitForDistributorToFinish()
{
    if (pthread_join(tIdDis, (void *)&statusDis) != 0)
    {
        perror("error on waiting for distributor");
        exit(EXIT_FAILURE);
    }
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("[%02d:%02d:%02d] MAIN: Thread Distributor has terminated!\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void waitForWorkersToFinish()
{
    int i;
    for (i = 0; i < nThreads; i++)
    {
        if (pthread_join(tIdWor[i], (void *)&statusWor[i]) != 0)
        {
            perror("error on waiting for worker");
            exit(EXIT_FAILURE);
        }
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        printf("[%02d:%02d:%02d] MAIN: Thread Worker %d has terminated!\n", tm.tm_hour, tm.tm_min, tm.tm_sec, i);
    }
}

int main(int argc, char *argv[])
{
    int opt;
    char *fileName = NULL;

    if (argc < 5)
    {
        printf("Usage: %s -f <file> -t <threads>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    while ((opt = getopt(argc, argv, "f:t:h")) != -1)
    {
        switch (opt)
        {
        case 'f':
            fileName = optarg;
            break;
        case 't':
            nThreads = atoi(optarg);
            break;
        case 'h':
            printf("Usage: %s -f <file> -t <threads>\n", argv[0]);
            exit(EXIT_SUCCESS);
        default:
            printf("Usage: %s -f <file> -t <threads>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    putFileName(fileName);

    if (((statusWor = malloc(nThreads * sizeof(int))) == NULL))
    {
        printf("error on allocating memory for worker status\n");
        exit(EXIT_FAILURE);
    }

    if ((tIdWor = malloc(nThreads * sizeof(pthread_t))) == NULL)
    {
        printf("error on allocating memory for worker threads\n");
        exit(EXIT_FAILURE);
    }

    workerId = malloc(nThreads * sizeof(int));
    int i;
    for (i = 0; i < nThreads; i++)
    {
        workerId[i] = i;
    }

    (void)get_delta_time();

    initializeDistributor();
    initializeWorkers();

    waitForWorkersToFinish();
    waitForDistributorToFinish();

    int *array;
    getArray(&array, 0, fullSize);

    validateArray(array, fullSize, sortType);

    printf("Time elapsed: %f s\n", get_delta_time());

    free(array);
    free(statusWor);
    free(tIdWor);

    exit(EXIT_SUCCESS);
}