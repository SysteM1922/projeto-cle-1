#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>

#include "bitonicSort.c"

int main(int argc, char *argv[])
{

    FILE *file;
    int size;
    int numberOfThreads = 4;
    int sortType = 0;

    file = fopen(argv[1], "rb");

    if (file == NULL)
    {
        printf("Erro ao abrir o arquivo %s\n", argv[1]);
        return 1;
    }

    if (fread(&size, sizeof(int), 1, file) != 1)
    {
        printf("Erro ao ler o tamanho do arquivo %s\n", argv[1]);
        return 1;
    }

    int startSize = size;

    int *array = (int *)malloc(size * sizeof(int));

    if (fread(array, sizeof(int), size, file) != size)
    {
        printf("Erro ao ler o arquivo %s\n", argv[1]);
        return 1;
    }
    
    size /= numberOfThreads;
    
    int exponent = log2(numberOfThreads);

    int i;
    for (i = 0; i < numberOfThreads; i++)
    {
        sortArray(array + size*i, size, i % 2 == sortType);
    }
    size *= 2;
    numberOfThreads /= 2;

    int j;
    for (i = 0; i < exponent; i++)
    {
        for (j = 0; j < numberOfThreads; j++) {
            order(array + size*j, size, j % 2 == sortType);
        }
        size *= 2;
        numberOfThreads /= 2;
    }

    validateArray(array, startSize, sortType);

    fclose(file);

    return 0;
}