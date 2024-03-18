#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>

#include "bitonicSort.c"

int statusDis;

int *statusWor;

int nThreads;

char *fileName;

int sortType = 0;

int startSize;

int *array;

static void *distributor();

static void *worker(void *par);

int main(int argc, char *argv[])
{

    fileName = argv[1];
    nThreads = atoi(argv[2]);

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

    validateArray(array, startSize, sortType);

    exit(EXIT_SUCCESS);
}

static void *distributor()
{
    FILE *file;
    int size;

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

    startSize = size;

    array = (int *)malloc(size * sizeof(int));

    if (fread(array, sizeof(int), size, file) != size)
    {
        printf("Erro ao ler o arquivo %s\n", fileName);
        statusDis = EXIT_FAILURE;
        pthread_exit(&statusDis);
    }

    fclose(file);

    size /= nThreads;

    int exponent = log2(nThreads);

    int i;
    for (i = 0; i < nThreads; i++)
    {
        sortArray(array + size * i, size, i % 2 == sortType);
    }
    size *= 2;
    nThreads /= 2;

    int j;
    for (i = 0; i < exponent; i++)
    {
        for (j = 0; j < nThreads; j++)
        {
            order(array + size * j, size, j % 2 == sortType);
        }
        size *= 2;
        nThreads /= 2;
    }

    statusDis = EXIT_SUCCESS;
    pthread_exit(&statusDis);
}

static void *worker(void *par)
{
    exit(EXIT_SUCCESS);
}