#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <stdbool.h>

#include "structs.h"

void putArray(int *array, int start, int size);

void getArray(int **array, int start, int size);

void getFullArray(int **array);

void putFileName(char *fileName);

void getFileName(char **fileName);

void distributeSubArrays(SubArray *arraySub, int *fullArray, int subSize, int fullSize);

void completeWork(int id);

bool sortCompleted();

int notFinished(int id);

void requestWork(int id, SubArray *subArray);

void waitForWorkRequest();

void waitForWorkComplete();

void initializeSharedMemory();

void mutex_lock();

void mutex_unlock();

#endif