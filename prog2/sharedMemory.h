/**
 * \file sharedMemory.h
 * 
 * \brief Shared Memory module.
 * 
 * This module provides the shared memory functions.
 * 
 * \author Guilherme Antunes - 103600
 * \author Pedro Rasinhas - 103541
*/

#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <stdbool.h>

#include "structs.h"

/**
 * \brief Function putArray.
 * 
 * Puts the array in the shared memory.
 * 
 * \param array array
 * \param start start index
 * \param size array size
 */
void putArray(int *array, int start, int size);

/**
 * \brief Function getArray.
 * 
 * Gets the array from the shared memory.
 * 
 * \param array array
 * \param start start index
 * \param size array size
 */
void getArray(int **array, int start, int size);

/**
 * \brief Function getFullArray.
 * 
 * Gets the full array from the shared memory.
 * 
 * \param array full array
 */
void getFullArray(int **array);

/**
 * \brief Function putFileName.
 * 
 * Puts the file name in the shared memory.
 * 
 * \param file file name
 */
void putFileName(char *fileName);

/**
 * \brief Function getFileName.
 * 
 * Gets the file name from the shared memory.
 * 
 * \param file file name
 */
void getFileName(char **fileName);

/**
 * \brief Function distributeSubArrays.
 * 
 * Put the sub arrays in the shared memory.
 * 
 * \param arraySub sub arrays
 * \param array full array
 * \param subSize sub array size
 * \param fullSize full array size
 */
void distributeSubArrays(SubArray *arraySub, int *fullArray, int subSize, int fullSize);

/**
 * \brief Function completeWork.
 * 
 * Completes the work.
 * 
 * \param id worker id
 */
void completeWork(int id);

/**
 * \brief Function sortCompleted.
 * 
 * Checks if the sort is completed.
 * 
 * \return true if the sort is completed, false otherwise
 */
bool sortCompleted();

/**
 * \brief Function notFinished.
 * 
 * Checks if the worker is not finished.
 * 
 * \param id worker id
 * \return true if the worker is not finished, false otherwise
 */
int notFinished(int id);

/**
 * \brief Function requestWork.
 * 
 * Requests work to the distributor.
 * 
 * \param id worker id
 * \param subArray sub array
 */
void requestWork(int id, SubArray *subArray);

/**
 * \brief Function getWorkerWork.
 * 
 * Gets the worker work.
 * 
 * \param id worker id
 * \return sub array
 */
void waitForWorkRequest();

/**
 * \brief Function waitForWorkComplete.
 * 
 * Distributor waits for work completion.
 */
void waitForWorkComplete();

/**
 * \brief Function putFileName.
 * 
 * Puts the file name in the shared memory.
 * 
 * \param file file name
 */
void initializeSharedMemory();

/**
 * \brief Function mutex_lock.
 * 
 * Locks the access to the shared memory.
 */
void mutex_lock();

/**
 * \brief Function mutex_unlock.
 * 
 * Unlocks the access to the shared memory.
 */
void mutex_unlock();

#endif