#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#include "bitonicSort.h"
#include "structs.h"
#include "sharedMemory.h"

/** \brief worker threads return status array */
extern int *statusWor;

/**
 *  \brief Function worker.
 *
 *  Its role is to simulate the life cycle of a worker.
 *
 *  \param par pointer to application defined worker identification
 */

void *worker(void *par)
{
    unsigned int id = *((unsigned int *)par);   /* worker id */
    int *array;     /* array to be sorted */

    SubArray data;  /* task data */

    time_t t;       /* current time */
    struct tm tm;   /* time structure */

    while (true)
    {
        requestWork(id, &data); /* request work */

        if (notFinished(id))    /* check if worker should terminate */
        {
            t = time(NULL);
            tm = *localtime(&t);
            printf("[%02d:%02d:%02d] WORKER %d: Received work\n", tm.tm_hour, tm.tm_min, tm.tm_sec, id);

            getArray(&array, data.start, data.size);    /* get array to sort from shared memory */

            if (data.action == SORT)
            {
                sort(array, data.size, data.sortType);      /* sort array */
            }
            else if (data.action == MERGE)
            {
                merge(array, data.size, data.sortType);     /* merge array */
            }

            putArray(array, data.start, data.size);     /* put array back to shared memory */


            t = time(NULL);
            tm = *localtime(&t);
            printf("[%02d:%02d:%02d] WORKER %d: Completed work\n", tm.tm_hour, tm.tm_min, tm.tm_sec, id);

            completeWork(id);   /* signal work completion */
        }
        else
        {
            break;  /* terminate worker */
        }
    }

    t = time(NULL);
    tm = *localtime(&t);
    printf("[%02d:%02d:%02d] WORKER %d: Terminating\n", tm.tm_hour, tm.tm_min, tm.tm_sec, id);

    statusWor[id] = EXIT_SUCCESS;
    pthread_exit(&statusWor[id]);   /* terminate thread */
}