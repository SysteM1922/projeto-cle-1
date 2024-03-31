#include "sharedRegion.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <wctype.h>
#include <wchar.h>

File fileStats[MAX_FILES]; /**< Array to store statistics for each file */
pthread_mutex_t fileStatsMutex = PTHREAD_MUTEX_INITIALIZER; /**< Mutex for file statistics */
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER; /**< Mutex for queue */

/**
 * @brief Initializes file structures.
 * 
 * Initializes the file structures for each file in the given array of filenames.
 * 
 * @param filenames Array of filenames
 * @param numFiles Number of filenames in the array
 */
void initFileStructures(char **filenames, int numFiles)
{
    for (int i = 0; i < numFiles; i++)
    {
        fileStats[i].filename = filenames[i];
        fileStats[i].fileSize = 0;
        fileStats[i].wordsCount = 0;
        fileStats[i].wordsWithConsonants = 0;
    }
}

/**
 * @brief Initializes a FIFO queue.
 * 
 * Initializes the given FIFO queue with default values.
 * 
 * @param q Pointer to the FIFO queue
 */
void initQueue(Queue *q)
{
    q->front = NULL;
    q->rear = NULL;

    pthread_mutex_init(&q->mutex, NULL);
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_mutex_init(&fileStatsMutex, NULL);
}

/**
 * @brief Enqueues chunks into the FIFO queue.
 * 
 * Enqueues chunks of data into the FIFO queue for processing by worker threads, making sure that the chunks dont end in the middle of a word.
 * 
 * @param q Pointer to the FIFO queue
 * @param filenames Array of filenames
 * @param numFiles Number of filenames in the array
 * @param threadSize Size of thread's chunk
 */
void enqueueChunks(Queue *q, char **filenames, int numFiles, int threadSize)
{
    // Lock the mutex before accessing the queue
    if (pthread_mutex_lock(&queue_mutex) != 0)
    {
        perror("Error locking mutex");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < numFiles; i++)
    {
        struct stat st;
        if (stat(filenames[i], &st) == 0)
        {
            int fileSize = st.st_size;
            int chunks = (fileSize + threadSize - 1) / threadSize; // Calculate number of chunks
            printf("File: %s [%d] (%d)", filenames[i], fileSize, chunks);
            printf("\n");
            int start = 0;

            if (q->rear != NULL && strcmp(q->rear->chunk->filename, filenames[i]) == 0)
            {
                start = q->rear->chunk->end + 1;
            }

            for (int j = 0; j < chunks; j++)
            {
                int end = (j == chunks - 1) ? fileSize : start + threadSize - 1;
                Chunk *chunk = (Chunk *)malloc(sizeof(Chunk));
                chunk->filename = strdup(filenames[i]);
                chunk->start = start;
                chunk->wordsCount = 0;
                chunk->wordsWithConsonants = 0;
                // before enqueuing the chunk, check if the chunk is "perfect" (ends outside of a word) -> if not, adjust the end
                FILE *file = fopen(filenames[i], "rb");
                if (file == NULL)
                {
                    perror("Error opening file");
                    return;
                }

                // Seek to the end of the chunk
                fseek(file, end, SEEK_SET);

                wchar_t c;
                while ((c = fgetwc(file)) != WEOF)
                {
                    if ((iswalnum(c) || c == L'_' || c == L'’' || c == L'‘' || c == L'\''))
                    {
                        printf("Found a word character at the end of the chunk [%d-%d] in file %s\n", start, end, filenames[i]);
                        printf("Previous end: %d\n", end);
                        end++;
                        printf("New end: %d\n", end);
                    }
                    else
                    {
                        break;
                    }
                }
                printf("\n");
                printf("Enqueuing chunk from file %s [%d-%d]\n", chunk->filename, chunk->start, end);
                chunk->end = end;
                enqueue(q, chunk);
                start = end + 1;
            }
        }
        else
        {
            perror("Error determining file size");
            exit(EXIT_FAILURE);
        }
    }
    // Unlock the mutex after accessing the queue
    if (pthread_mutex_unlock(&queue_mutex) != 0)
    {
        perror("Error unlocking mutex");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Enqueues a chunk into the FIFO queue.
 * 
 * Enqueues the given chunk into the FIFO queue.
 * 
 * @param q Pointer to the FIFO queue
 * @param chunk Pointer to the chunk to enqueue
 */
void enqueue(Queue *q, Chunk *chunk)
{

    if (pthread_mutex_lock(&q->mutex) != 0)
    {
        perror("Error locking mutex");
        exit(EXIT_FAILURE);
    }

    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->chunk = chunk;
    newNode->next = NULL;

    if (q->rear == NULL)
    {
        q->front = newNode;
        q->rear = newNode;
    }
    else
    {
        q->rear->next = newNode;
        q->rear = newNode;
    }

    if (pthread_mutex_unlock(&q->mutex) != 0)
    {
        perror("Error unlocking mutex");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Dequeues a chunk from the FIFO queue.
 * 
 * Dequeues a chunk from the FIFO queue and returns a pointer to it.
 * 
 * @param q Pointer to the FIFO queue
 * @return Pointer to the dequeued chunk
 */
Chunk *dequeue(Queue *q, pthread_t threadID)
{
    if (pthread_mutex_lock(&q->mutex) != 0)
    {
        perror("Error locking mutex");
        exit(EXIT_FAILURE);
    }

    if (q->front == NULL)
    {
        if (pthread_mutex_unlock(&q->mutex) != 0)
        {
            perror("Error unlocking mutex");
            exit(EXIT_FAILURE);
        }
        return NULL;
    }
    else
    {
        Node *temp = q->front;
        Chunk *chunk = temp->chunk;
        q->front = q->front->next;
        free(temp);

        if (q->front == NULL)
        {
            q->rear = NULL;
        }

        printf("Thread %ld, dequeued chunk from file %s [%d-%d]\n\n", threadID, chunk->filename, chunk->start, chunk->end);

        if (pthread_mutex_unlock(&q->mutex) != 0)
        {
            perror("Error unlocking mutex");
            exit(EXIT_FAILURE);
        }
        return chunk;
    }
}

/**
 * @brief Update the file statistics.
 * 
 * Update the statistics for the specified file with the given word counts.
 * 
 * @param filename Name of the file
 * @param numWords Number of words processed
 * @param numWordsWithConsonants Number of words with consonants processed
 * @param threadID ID of the thread performing the update
 */
void updateFileStats(char *filename, int numWords, int numWordsWithConsonants, pthread_t threadID)
{
    if (pthread_mutex_lock(&fileStatsMutex) != 0)
    {
        perror("Error locking mutex");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (strcmp(fileStats[i].filename, filename) == 0)
        {
            // printf("Thread %lu has finished processing %d words and %d words with consonants\n", threadID, numWords, numWordsWithConsonants);
            fileStats[i].wordsCount += numWords;
            fileStats[i].wordsWithConsonants += numWordsWithConsonants;
            break;
        }
    }
    if (pthread_mutex_unlock(&fileStatsMutex) != 0)
    {
        perror("Error unlocking mutex");
        exit(EXIT_FAILURE);
    }
}
