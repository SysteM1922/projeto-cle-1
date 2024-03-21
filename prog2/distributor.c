#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#include "main.h"
#include "sharedMemory.h"

static void *distributor(void *par)
{
    FILE *file;
    int size;

    struct distributor_args *args = (struct distributor_args *)par;

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

    int *array = (int *)malloc(size * sizeof(int));

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
        sort(array + size * i, size, i % 2 == sortType);
    }
    size *= 2;
    nThreads /= 2;

    int j;
    for (i = 0; i < exponent; i++)
    {
        for (j = 0; j < nThreads; j++)
        {
            merge(array + size * j, size, j % 2 == sortType);
        }
        size *= 2;
        nThreads /= 2;
    }

    statusDis = EXIT_SUCCESS;
    pthread_exit(&statusDis);
}