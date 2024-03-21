#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>

#include "bitonicSort.h"
#include "main.h"
#include "distributor.c"
#include "worker.c"
#include "sharedMemory.h"

int statusDis;

int *statusWor;

int sortType = 0;

int main(int argc, char *argv[])
{
    int startSize;

    char *fileName = argv[1];
    int nThreads = atoi(argv[2]);

    pthread_t tIdDis;
    unsigned int dis;

    pthread_t *tIdWor;
    unsigned int *wor;

    if (((tIdWor = malloc(nThreads * sizeof(pthread_t))) == NULL) ||
        ((wor = malloc(nThreads * sizeof(unsigned int))) == NULL))
    {
        printf("something\n");
        exit(EXIT_FAILURE);
    }

    dis = 0;
    int i;
    for (i = 0; i < nThreads; i++)
    {
        wor[i] = i;
    }

    if (pthread_create(&tIdDis, NULL, distributor, NULL) != 0)
    {
        perror("error on creating thread distributor");
        exit(EXIT_FAILURE);
    }

    if (pthread_join(tIdDis, (void *)&statusDis) != 0)
    {
        perror("error on waiting for thread distributor");
        exit(EXIT_FAILURE);
    }

    int *array;
    getArray(0, &array);

    validateArray(array, startSize, sortType);

    exit(EXIT_SUCCESS);
}