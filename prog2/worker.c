#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#include "bitonicSort.h"
#include "structs.h"
#include "sharedMemory.h"

extern int *statusWor;

void *worker(void *par)
{
    unsigned int id = *((unsigned int *)par);
    int *array;

    SubArray data;

    time_t t;
    struct tm tm;

    while (true)
    {
        requestWork(id, &data);

        if (notFinished(id))
        {
            t = time(NULL);
            tm = *localtime(&t);
            printf("[%02d:%02d:%02d] WORKER %d: Received work\n", tm.tm_hour, tm.tm_min, tm.tm_sec, id);

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


            t = time(NULL);
            tm = *localtime(&t);
            printf("[%02d:%02d:%02d] WORKER %d: Completed work\n", tm.tm_hour, tm.tm_min, tm.tm_sec, id);

            completeWork(id);
        }
        else
        {
            break;
        }
    }

    t = time(NULL);
    tm = *localtime(&t);
    printf("[%02d:%02d:%02d] WORKER %d: Terminating\n", tm.tm_hour, tm.tm_min, tm.tm_sec, id);

    statusWor[id] = EXIT_SUCCESS;
    pthread_exit(&statusWor[id]);
}