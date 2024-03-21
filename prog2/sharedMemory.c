#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

char *fileName;

static pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;

extern int statusDistributor;

void putFileName(char *fileName)
{
    if ((statusDistributor = pthread_mutex_lock(&accessCR)) != 0)
    {
        errno = statusDistributor;
        perror("error on entering monitor(CF)");
        statusDistributor = EXIT_FAILURE;
        pthread_exit(&statusDistributor);
    }

    fileName = fileName;
}

void putArray(int start, int *array)
{
    start = (int)array;
}

void getArray(int start, int **array)
{
    *array = (int *)start;
}