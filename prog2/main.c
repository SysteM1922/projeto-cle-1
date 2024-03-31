/**
 * \file main.c
 * 
 * \brief Main module.
 * 
 * This module provides the program's logic.
 * 
 * \author Guilherme Antunes - 103600
 * \author Pedro Rasinhas - 103541
*/

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

/** \brief sort type */
int sortType = 0;

/** \brief full array size */
int fullSize;

/** \brief number of threads available at the beginning of the program */
int nThreads;

/** \brief distributor thread return status */
int statusDis;

/** \brief worker threads return status */
int *statusWor;

/** \brief worker ids */
int *workerId;

/** \brief distributor thread id */
pthread_t tIdDis;

/** \brief worker threads id */
pthread_t *tIdWor;

/** \brief distributor arguments */
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

/**
 * \brief Function to start the distributor thread.
 */

void initializeDistributor()
{
    distributor_args.sortType = sortType;

    if (pthread_create(&tIdDis, NULL, distributor, &distributor_args) != 0)
    {
        perror("error on creating thread distributor");
        exit(EXIT_FAILURE);
    }
}

/**
 * \brief Function to start the worker threads.
 */

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

/**
 * \brief Function to wait for the distributor thread to finish.
 */

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

/**
 * \brief Function to wait for the worker threads to finish.
 */

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

/**
 *  \brief Main thread.
 *
 *  Its role is starting the simulation by generating the intervening entities threads (workers and distributor) and
 *  waiting for their termination.
 *
 *  \param argc number of words of the command line
 *  \param argv list of words of the command line
 *
 *  \return status of operation
 */

int main(int argc, char *argv[])
{
    int opt;                /* option */
    char *fileName = NULL;  /* file name */

    if (argc < 5)
    {
        printf("Usage: %s -f <file> -t <threads>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    while ((opt = getopt(argc, argv, "f:t:h")) != -1)
    {
        switch (opt)
        {
        case 'f':   /* file name */
            fileName = optarg;
            break;
        case 't':   /* number of threads */
            nThreads = atoi(optarg);
            break;
        case 'h':   /* help */
            printf("Usage: %s -f <file> -t <threads>\n", argv[0]);
            exit(EXIT_SUCCESS);
        default:
            printf("Usage: %s -f <file> -t <threads>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    putFileName(fileName);      /* put file name in shared memory */

    if (((statusWor = malloc(nThreads * sizeof(int))) == NULL))     /* allocate memory for worker status */
    {
        printf("error on allocating memory for worker status\n");
        exit(EXIT_FAILURE);
    }

    if ((tIdWor = malloc(nThreads * sizeof(pthread_t))) == NULL)    /* allocate memory for worker threads */
    {
        printf("error on allocating memory for worker threads\n");
        exit(EXIT_FAILURE);
    }

    workerId = malloc(nThreads * sizeof(int));      /* allocate memory for worker ids */
    int i;
    for (i = 0; i < nThreads; i++)
    {
        workerId[i] = i;    /* set worker ids */
    }

    (void)get_delta_time();     /* get initial time */

    initializeDistributor();    /* initialize distributor */
    initializeWorkers();        /* initialize workers */

    waitForWorkersToFinish();       /* wait for workers to finish */
    waitForDistributorToFinish();   /* wait for distributor to finish */

    int *array;     /* array */
    getArray(&array, 0, fullSize);      /* get array from shared memory */

    validateArray(array, fullSize, sortType);   /* validate array */

    printf("Time elapsed: %f s\n", get_delta_time());

    free(array);
    free(statusWor);
    free(tIdWor);

    exit(EXIT_SUCCESS);    /* exit */
}