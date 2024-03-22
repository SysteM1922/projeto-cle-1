#include "sharedRegion.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

File fileStats[MAX_FILES];
pthread_mutex_t fileStatsMutex = PTHREAD_MUTEX_INITIALIZER;

void initFileStructures(char** filenames, int numFiles) {
    for (int i = 0; i < numFiles; i++) {
        fileStats[i].filename = filenames[i];
        fileStats[i].fileSize = 0; // This will be updated later
        fileStats[i].wordsCount = 0;
        fileStats[i].wordsWithConsonants = 0;
        fileStats[i].isFinished = 0;
    }
}

// Initialize FIFO queue
void initQueue(Queue* q) {
    q->front = NULL;
    q->rear = NULL;
    pthread_mutex_init(&q->mutex, NULL);
}

void enqueueChunks(Queue* q, char** filenames, int numFiles, int threadSize) {
    for (int i = 0; i < numFiles; i++) {
        struct stat st;
        if (stat(filenames[i], &st) == 0) {
            int fileSize = st.st_size;
            int chunks = (fileSize + threadSize - 1) / threadSize; // Calculate number of chunks
            int start = 0;
            for (int j = 0; j < chunks; j++) {
                int end = (j == chunks - 1) ? fileSize : start + threadSize - 1;
                Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
                chunk->filename = strdup(filenames[i]);
                chunk->start = start;
                chunk->end = end;
                enqueue(q, chunk);
                start = end + 1;
            }
        } else {
            perror("Error determining file size");
            exit(EXIT_FAILURE);
        }
    }
}


// Enqueue a chunk into the FIFO queue
void enqueue(Queue* q, Chunk* chunk) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->chunk = chunk;
    newNode->next = NULL;
    
    pthread_mutex_lock(&q->mutex);
    
    if (q->rear == NULL) {
        q->front = newNode;
        q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }
    
    pthread_mutex_unlock(&q->mutex);
}

// Dequeue a chunk from the FIFO queue
Chunk* dequeue(Queue* q) {
    pthread_mutex_lock(&q->mutex);
    
    if (q->front == NULL) {
        pthread_mutex_unlock(&q->mutex);
        return NULL;
    } else {
        Node* temp = q->front;
        Chunk* chunk = temp->chunk;
        q->front = q->front->next;
        free(temp);
        
        if (q->front == NULL) {
            q->rear = NULL;
        }
        
        pthread_mutex_unlock(&q->mutex);
        return chunk;
    }
}

// Update the file statistics
void updateFileStats(int fileIndex, int numWords, int numWordsWithConsonants) {
    pthread_mutex_lock(&fileStatsMutex);
    fileStats[fileIndex].wordsCount += numWords;
    fileStats[fileIndex].wordsWithConsonants += numWordsWithConsonants;
    pthread_mutex_unlock(&fileStatsMutex);
}
