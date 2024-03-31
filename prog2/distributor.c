#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#include "structs.h"
#include "distributor.h"
#include "sharedMemory.h"

/** \brief full array size */
extern int fullSize;

/** \brief distributor thread return status */
extern int statusDis;

/** \brief number of threads available at the beginning of the program */
extern int nThreads;

/**
 *  \brief Function worker.
 *
 *  Its role is to simulate the life cycle of the distributor.
 *
 *  \param par pointer to passed distributor arguments
 */

void *distributor(void *par)
{
    FILE *file;     /* file pointer */
    int size;       /* file size */

    time_t t;       /* current time */
    struct tm tm;   /* time structure */

    DistributorArgs *args = (DistributorArgs *)par;     /* distributor arguments */

    int sortType = args->sortType;      /* sort type */

    char *fileName;     /* file name */
    getFileName(&fileName);     /* get file name from shared memory */

    file = fopen(fileName, "rb");       /* open file */

    if (file == NULL)       /* check if file was opened */
    {
        printf("Erro ao abrir o arquivo %s\n", fileName);
        statusDis = EXIT_FAILURE;
        pthread_exit(&statusDis);
    }

    if (fread(&size, sizeof(int), 1, file) != 1)        /* read file size */
    {
        printf("Erro ao ler o tamanho do arquivo %s\n", fileName);
        statusDis = EXIT_FAILURE;
        pthread_exit(&statusDis);
    }

    fullSize = size;        /* set full array size */
    int *array = (int *)malloc(size * sizeof(int));     /* allocate array */

    if (fread(array, sizeof(int), size, file) != size)      /* read array */
    {
        printf("Erro ao ler o arquivo %s\n", fileName);
        statusDis = EXIT_FAILURE;
        pthread_exit(&statusDis);
    }

    fclose(file);       /* close file */

    int exponent = log2(nThreads);      /* calculate exponent of nThreads */

    SubArray *subArrays = (SubArray *)malloc((nThreads * 2 - 1) * sizeof(SubArray));        /* allocate subArrays */

    int idx = 0;    /* subArray index */
    int i;
    int j;
    for (i = 0; i < exponent; i++)      /* make exponent number of iteractions */
    {
        for (j = 0; j < pow(2, i); j++)         /* create subArrays doubling size each new cycle for merging */
        {
            subArrays[idx].id = idx;            /* set subArray id */
            subArrays[idx].start = j * size;    /* set subArray start */
            subArrays[idx].size = size;         /* set subArray size */
            subArrays[idx].sortType = j % 2 == sortType;    /* set subArray sort type */
            subArrays[idx].dependent_1 = idx*2 + 1;         /* set subArray dependent 1 */
            subArrays[idx].dependent_2 = idx*2 + 2;         /* set secondsubArray dependent 2 */
            subArrays[idx].threadId = pow(2, exponent - i) * j;     /* adress the subArray to the thread */
            subArrays[idx].completed = false;       /* set subArray completed */
            subArrays[idx].action = MERGE;          /* set subArray action */
            idx++;      /* update index */
        }
        size /= 2;      /* update size for next iteration */
    }

    for (i = 0; i < nThreads; i++)      /* create subArrays for sorting */
    {
        subArrays[idx].id = idx;            /* set subArray id */
        subArrays[idx].start = i * size;    /* set subArray start */
        subArrays[idx].size = size;         /* set subArray size */
        subArrays[idx].sortType = i % 2 == sortType;        /* set subArray sort type */
        subArrays[idx].dependent_1 = -1;            /* set subArray dependent 1 null */
        subArrays[idx].dependent_2 = -1;            /* set subArray dependent 2 null */
        subArrays[idx].threadId = i;                /* adress the subArray to the thread */
        subArrays[idx].completed = false;       /* set subArray completed */
        subArrays[idx].action = SORT;           /* set subArray action */
        idx++;      /* update index */
    }

    distributeSubArrays(subArrays, array, nThreads * 2 - 1, fullSize);      /* store subarrays in shared memory */

    free(array);        /* free array */
    free(subArrays);    /* free subArrays */

    while (sortCompleted() != true)     /* wait for work to be completed */
    {
        waitForWorkRequest();           /* wait for works request */
        t = time(NULL);
        tm = *localtime(&t);
        printf("[%02d:%02d:%02d] DISTRIBUTOR: Received work request\n", tm.tm_hour, tm.tm_min, tm.tm_sec);

        waitForWorkComplete();          /* wait for works to be completed */
        t = time(NULL);
        tm = *localtime(&t);
        printf("[%02d:%02d:%02d] DISTRIBUTOR: Received work complete\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
    }

    t = time(NULL);
    tm = *localtime(&t);
    printf("[%02d:%02d:%02d] DISTRIBUTOR: Terminating\n", tm.tm_hour, tm.tm_min, tm.tm_sec);

    statusDis = EXIT_SUCCESS;
    pthread_exit(&statusDis);       /* exit thread */
}