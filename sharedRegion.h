#ifndef SHARED_REGION_H
#define SHARED_REGION_H

#include <pthread.h>
#include "args.h"

typedef struct {
    char* filename;           
    int fileSize;             
    int wordsCount;           
    int wordsWithConsonants; 
    int isFinished;           
} File;

typedef struct {
    char* filename;  
    int start;       
    int end;         
    int wordsCount; // Number of words processed by the thread for this chunk
    int wordsWithConsonants; // Number of words with at least two instances of the same consonant
} Chunk;

typedef struct Node {
    Chunk* chunk;
    struct Node* next;
} Node;

typedef struct {
    Node* front;
    Node* rear;
    pthread_mutex_t mutex;
} Queue;

typedef struct {
    pthread_t threadID;             
    Queue* fifo;                    
    File* fileStats;                
} ThreadData;

extern File fileStats[MAX_FILES];
extern pthread_mutex_t fileStatsMutex;

void initFileStructures(char** filenames, int numFiles);
void enqueueChunks(Queue* q, char** filenames, int numFiles, int threadSize);

void initQueue(Queue* q);
void enqueue(Queue* q, Chunk* chunk);
Chunk* dequeue(Queue* q);

void updateFileStats(char* filename, int numWords, int numWordsWithConsonants, pthread_t threadID);

void aggregateFileStats(int numFiles);

#endif // SHARED_REGION_H