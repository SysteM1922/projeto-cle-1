#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "main.h"
#include "sharedMemory.h"
#include "bitonicSort.h"

static void *worker(void *par)
{
    struct work_args *args = (struct work_args *)par;

    int *array;

    while (1)
    {
        switch (args->action)
        {
        case WAITING:
            break;
        case SORT:
            getArray(args->start, &array);
            sort(array, args->size, args->sortType);
            args->action = WAITING;
            break;
        case MERGE:
            getArray(args->start, &array);
            merge(array, args->size, args->sortType);
            args->action = WAITING;
            break;
        case FINISH:
            exit(EXIT_SUCCESS);
        default:
            exit(EXIT_FAILURE);
        }
    }
}