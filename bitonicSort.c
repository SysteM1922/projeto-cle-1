#include <stdio.h>
#include <stdlib.h>

void swap(int *a, int *b, int sortType)
{
    if (sortType == (*a > *b))
    {
        int temp = *a;
        *a = *b;
        *b = temp;
    }
}

void order(int *array, int size, int sortType)
{
    if (size > 1)
    {
        int i;
        int half = size / 2;
        for (i = 0; i < half; i++)
        {
            swap(&array[i], &array[i + half], sortType);
        }
        order(array, half, sortType);
        order(array + half, size - half, sortType);
    }
}

void sortArray(int *array, int size, int sortType)
{
    if (size > 1)
    {
        int half = size / 2;
        sortArray(array, half, 1);
        sortArray(array + half, size - half, 0);
        order(array, size, sortType);
    }
}

void validateArray(int *array, int size, int sortType)
{
    int j;
    for (j = 0; j < size - 1; j++)
    {
        if (sortType == (array[j] < array[j + 1]) && array[j] != array[j + 1])
        {
            printf("Error in position %d between element %d and %d\n", j, array[j], array[j + 1]);
            break;
        }
    }
    if (j == (size - 1))
    {
        printf("Everything is OK!\n");
    }
}