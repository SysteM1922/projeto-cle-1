#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#include "structs.h"
#include "distributor.h"
#include "sharedMemory.h"

extern int fullSize;

extern int statusDis;

extern int nThreads;

void *distributor(void *par)
{
    FILE *file;
    int size;

    time_t t;
    struct tm tm;

    DistributorArgs *args = (DistributorArgs *)par;

    int sortType = args->sortType;

    char *fileName;
    getFileName(&fileName);

    file = fopen(fileName, "rb");

    if (file == NULL)
    {
        printf("Erro ao abrir o arquivo %s\n", fileName);
        statusDis = EXIT_FAILURE;
        pthread_exit(&statusDis);
    }

    if (fread(&size, sizeof(int), 1, file) != 1)
    {
        printf("Erro ao ler o tamanho do arquivo %s\n", fileName);
        statusDis = EXIT_FAILURE;
        pthread_exit(&statusDis);
    }

    fullSize = size;
    int *array = (int *)malloc(size * sizeof(int));

    if (fread(array, sizeof(int), size, file) != size)
    {
        printf("Erro ao ler o arquivo %s\n", fileName);
        statusDis = EXIT_FAILURE;
        pthread_exit(&statusDis);
    }

    fclose(file);

    int exponent = log2(nThreads);

    SubArray *subArrays = (SubArray *)malloc((nThreads * 2 - 1) * sizeof(SubArray));

    int idx = 0;
    int i;
    int j;
    for (i = 0; i < exponent; i++)
    {
        for (j = 0; j < pow(2, i); j++)
        {
            subArrays[idx].id = idx;
            subArrays[idx].start = j * size;
            subArrays[idx].size = size;
            subArrays[idx].sortType = j % 2 == sortType;
            subArrays[idx].dependent_1 = idx*2 + 1;
            subArrays[idx].dependent_2 = idx*2 + 2;
            subArrays[idx].threadId = pow(2, exponent - i) * j;
            subArrays[idx].completed = false;
            subArrays[idx].action = MERGE;
            idx++;
        }
        size /= 2;
    }

    for (i = 0; i < nThreads; i++)
    {
        subArrays[idx].id = idx;
        subArrays[idx].start = i * size;
        subArrays[idx].size = size;
        subArrays[idx].sortType = i % 2 == sortType;
        subArrays[idx].dependent_1 = -1;
        subArrays[idx].dependent_2 = -1;
        subArrays[idx].threadId = i;
        subArrays[idx].completed = false;
        subArrays[idx].action = SORT;
        idx++;
    }

    distributeSubArrays(subArrays, array, nThreads * 2 - 1, fullSize);

    free(array);
    free(subArrays);

    while (sortCompleted() != true)
    {
        waitForWorkRequest();
        t = time(NULL);
        tm = *localtime(&t);
        printf("[%02d:%02d:%02d] DISTRIBUTOR: Received work request\n", tm.tm_hour, tm.tm_min, tm.tm_sec);

        waitForWorkComplete();
        t = time(NULL);
        tm = *localtime(&t);
        printf("[%02d:%02d:%02d] DISTRIBUTOR: Received work complete\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
    }

    t = time(NULL);
    tm = *localtime(&t);
    printf("[%02d:%02d:%02d] DISTRIBUTOR: Terminating\n", tm.tm_hour, tm.tm_min, tm.tm_sec);

    statusDis = EXIT_SUCCESS;
    pthread_exit(&statusDis);
}