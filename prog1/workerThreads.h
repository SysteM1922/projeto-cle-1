#ifndef WORKER_THREADS_H
#define WORKER_THREADS_H

#include <pthread.h>
#include "sharedRegion.h" // Assuming shared_memory.h contains the definitions for File, Chunk, Queue, and ThreadData
#include <stdio.h>
#include <stdlib.h>

void createThreads(int numThreads, Queue* fifo, pthread_t* threads, ThreadData* threadData);
void joinThreads(int numThreads, pthread_t* threads);

// Thread start routine
void* threadStartRoutine(void* arg);

// Function to process a chunk
void processChunk(FILE* file, Chunk* chunk, pthread_t threadID);

#endif // WORKER_THREADS_H