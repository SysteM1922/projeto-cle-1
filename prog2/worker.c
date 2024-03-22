#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "bitonicSort.h"
#include "structs.h"
#include "sharedMemory.h"

extern int *statusWor;

void *worker(void *par)
{
    int id = *(int *)par;
    int *array;

    SubArray data;

    while (true)
    {
        requestWork(id, &data);

        if (notFinished(id))
        {
            getArray(&array, data.start, data.size);

            if (data.action == SORT)
            {
                sort(array, data.size, data.sortType);
            }
            else if (data.action == MERGE)
            {
                merge(array, data.size, data.sortType);
            }

            putArray(array, data.start, data.size);

            completeWork(id);
        }
        else
        {
            break;
        }
    }

    statusWor[id] = EXIT_SUCCESS;
    pthread_exit(&statusWor[id]);
}