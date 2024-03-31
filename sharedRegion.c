#include "sharedRegion.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <wctype.h>
#include <wchar.h>

File fileStats[MAX_FILES];
static pthread_once_t init = PTHREAD_ONCE_INIT;
static pthread_mutex_t fileStatsMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

void mutex_lock()
{
    if (pthread_mutex_lock(&queue_mutex) != 0)
    {
        perror("Error locking mutex");
        exit(EXIT_FAILURE);
    }
}

void mutex_unlock()
{
    if (pthread_mutex_unlock(&queue_mutex) != 0)
    {
        perror("Error unlocking mutex");
        exit(EXIT_FAILURE);
    }
}

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

// Initialize FIFO queue
void initQueue(Queue *q)
{
    q->front = NULL;
    q->rear = NULL;

    pthread_mutex_init(&q->mutex, NULL);
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_mutex_init(&fileStatsMutex, NULL);
}

void enqueueChunks(Queue *q, char **filenames, int numFiles, int threadSize)
{
    // Lock the mutex before accessing the queue
    mutex_lock();

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
                        end++;
                    }
                    else
                    {
                        break;
                    }
                }

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
    mutex_unlock();
}

// Enqueue a chunk into the FIFO queue
void enqueue(Queue *q, Chunk *chunk)
{
    mutex_lock();

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

    mutex_unlock();
}

// Dequeue a chunk from the FIFO queue
void dequeue(Queue *q, Chunk **retChunk)
{
    mutex_lock();

    if (q->front == NULL)
    {
        *retChunk = NULL;
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
        *retChunk = chunk;
    }
    mutex_unlock();
}

// Update the file statistics
void updateFileStats(char *filename, int numWords, int numWordsWithConsonants, pthread_t threadID)
{
    mutex_lock();

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
    
    mutex_unlock();
}
