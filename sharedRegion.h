#ifndef SHARED_REGION_H
#define SHARED_REGION_H

#include <pthread.h>
#include "args.h"

/**
 * @brief Structure representing file statistics.
 * 
 * This structure holds information about a file's statistics,
 * including filename, file size, and word counts.
 */
typedef struct {
    char* filename;              /**< Filename */
    int fileSize;                /**< File size */
    int wordsCount;              /**< Total number of words */
    int wordsWithConsonants;     /**< Number of words with at least two instances of the same consonant */
} File;

/**
 * @brief Structure representing a chunk of data within a file.
 * 
 * This structure contains information about a specific chunk of data within a file.
 * It includes the filename, the starting position, and the ending position of the chunk.
 */
typedef struct {
    char* filename;              /**< Filename */
    int start;                   /**< Start position of the chunk */
    int end;                     /**< End position of the chunk */
    int wordsCount;              /**< Number of words processed by the thread for this chunk */
    int wordsWithConsonants;     /**< Number of words with at least two instances of the same consonant */
} Chunk;

/**
 * @brief Node structure for a FIFO queue.
 * 
 * This structure represents a node in a FIFO queue, containing a Chunk pointer and a pointer to the next node.
 */
typedef struct Node {
    Chunk* chunk;                /**< Pointer to the chunk */
    struct Node* next;          /**< Pointer to the next node */
} Node;

/**
 * @brief FIFO queue structure.
 * 
 * This structure represents a FIFO queue for storing chunks.
 * It includes pointers to the front and rear nodes, as well as a mutex for thread safety.
 */
typedef struct {
    Node* front;                /**< Pointer to the front node of the queue */
    Node* rear;                 /**< Pointer to the rear node of the queue */
    pthread_mutex_t mutex;     /**< Mutex for thread safety */
} Queue;

/**
 * @brief Thread data structure.
 * 
 * This structure holds data associated with a thread,
 * including the thread ID, a pointer to the FIFO queue, and a pointer to the file statistics.
 */
typedef struct {
    pthread_t threadID;          /**< Thread ID */
    Queue* fifo;                 /**< Pointer to the FIFO queue */
    File* fileStats;             /**< Pointer to the file statistics */
} ThreadData;

extern File fileStats[MAX_FILES];               /**< Array of file statistics */
extern pthread_mutex_t fileStatsMutex;          /**< Mutex for file statistics */

void initFileStructures(char** filenames, int numFiles);
void enqueueChunks(Queue* q, char** filenames, int numFiles, int threadSize);

void initQueue(Queue* q);
void enqueue(Queue* q, Chunk* chunk);
Chunk* dequeue(Queue* q);

void updateFileStats(char* filename, int numWords, int numWordsWithConsonants, pthread_t threadID);

#endif // SHARED_REGION_H
