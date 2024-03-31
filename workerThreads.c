#include "workerThreads.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <wctype.h>
#include <wchar.h>
#include <locale.h>

#include "sharedRegion.h"

/**
 * @brief Check if a character is a letter or underscore.
 *
 * Checks if the given character is a letter (alphabetic character or underscore).
 *
 * @param c The character to check
 * @return 1 if the character is a letter or underscore, otherwise 0
 */
int isLetter(wchar_t c)
{
    return iswalnum(c) || c == L'_';
}

/**
 * @brief Check if a character is a consonant.
 *
 * Checks if the given character is a consonant.
 *
 * @param c The character to check
 * @return 1 if the character is a consonant, otherwise 0
 */
int isConsonant(wchar_t c)
{
    return wcschr(L"bcdfghjklmnpqrstvwxyz", c) != NULL;
}

/**
 * @brief Extracts a simple letter from a complex letter.
 *
 * Extracts a simple letter from a complex letter if applicable.
 *
 * @param letter Pointer to the letter to extract
 */
void extractLetter(wchar_t *letter)
{
    *letter = towlower(*letter);
    wchar_t *simpleLetters = L"c";
    wchar_t *complexLetters = L"ç";

    for (int i = 0; i < wcslen(complexLetters); i++)
    {
        if (complexLetters[i] == *letter)
        {
            *letter = simpleLetters[i];
            break;
        }
    }
}

/**
 * @brief Creates worker threads.
 *
 * Creates the specified number of worker threads using the given FIFO queue and thread data.
 *
 * @param numThreads Number of worker threads to create
 * @param fifo Pointer to the FIFO queue
 * @param threads Array to store thread IDs
 * @param threadData Array of thread data structures
 */
void createThreads(int numThreads, Queue *fifo, pthread_t *threads, ThreadData *threadData)
{
    for (int i = 0; i < numThreads; i++)
    {
        threadData[i].fifo = fifo;               // Directly assign the pointer to the Queue structure
        threadData[i].fileStats = &fileStats[0]; // Assuming fileStats is an array of File structures, assign the address of the first element

        if (pthread_create(&threads[i], NULL, threadStartRoutine, &threadData[i]) != 0)
        {
            perror("Error creating thread");
            exit(EXIT_FAILURE);
        }

        printf("Thread %lu has been started\n", threads[i]);
    }
}

/**
 * @brief Joins worker threads.
 *
 * Waits for the specified number of worker threads to finish execution and joins them.
 *
 * @param numThreads Number of worker threads to join
 * @param threads Array of thread IDs
 */
void joinThreads(int numThreads, pthread_t *threads)
{
    for (int i = 0; i < numThreads; i++)
    {
        if (pthread_join(threads[i], NULL) != 0)
        {
            perror("Error joining thread");
            exit(EXIT_FAILURE);
        }

        printf("Thread %lu has finished\n", threads[i]);
    }
}

/**
 * @brief Thread start routine.
 *
 * Start routine for worker threads. Handles the processing of chunks.
 *
 * @param arg Pointer to thread data
 * @return NULL
 */
void *threadStartRoutine(void *arg)
{
    ThreadData *data = (ThreadData *)arg;
    Queue *fifo = data->fifo;

    setlocale(LC_ALL, "");

    while (1)
    {
        Chunk *chunk = dequeue(fifo, pthread_self());
        if (chunk == NULL)
        {
            break; // No more chunks to process
        }

        FILE *file = fopen(chunk->filename, "rb");
        if (file == NULL)
        {
            perror("Error opening file");
            continue;
        }

        // Seek to the start of the chunk
        fseek(file, chunk->start, SEEK_SET);

        // Process the chunk
        // actually count the words and words with consonants
        processChunk(file, chunk, pthread_self());
    }

    pthread_exit((void *)pthread_self());

    return NULL;
}

/**
 * @brief Process a chunk.
 *
 * Processes a chunk of data from the file associated with the given file pointer.
 *
 * @param file File pointer to the input file
 * @param chunk Pointer to the chunk to process
 * @param threadID ID of the thread processing the chunk
 */
void processChunk(FILE *file, Chunk *chunk, pthread_t threadID)
{
    // Seek to the start of the chunk
    fseek(file, chunk->start, SEEK_SET);

    FILE *temp_file = file;

    int temp_end = chunk->end;
    int temp_start = chunk->start;

    int wordCount = 0;
    int inWord = 0;
    int matchWords = 0;
    wchar_t word[21];
    int word_len = 0;
    int found = 0;
    wchar_t c;
    while ((c = fgetwc(file)) != WEOF && ftell(file) <= chunk->end) // Ensure to stop at the end of the chunk
    {
        if (isLetter(c))
        {
            if (found)
            {
                continue;
            }
            else
            {
                extractLetter(&c);
                if (!inWord)
                {
                    wordCount++;
                    inWord = 1;
                }
                if (isConsonant(c))
                {
                    for (int i = 0; i < word_len; i++)
                    {
                        if (word[i] == c)
                        {
                            found = 1;
                            matchWords++;
                            break;
                        }
                    }
                    if (!found)
                    {
                        word[word_len] = c;
                        word_len++;
                    }
                }
            }
        }
        else if (c != L'’' && c != L'‘' && c != L'\'')
        {
            inWord = 0;
            found = 0;
            memset(word, '\0', sizeof(word));
            word_len = 0;
        }
    }

    chunk->wordsCount = wordCount;
    chunk->wordsWithConsonants = matchWords;

    if (wordCount == 0)
    {
        // print the entre chunk
        fseek(temp_file, temp_start, SEEK_SET);
        printf("temp_start: %d\n", temp_start);
        printf("temp_end: %d\n", temp_end);
        wchar_t c;

        while ((c = fgetwc(file)) != WEOF && ftell(file) <= chunk->end)
        {
            printf("Reading character: %lc at position: %ld\n", c, ftell(file));
        }
        printf("out of words\n");
        // wait 30 seconds
        // sleep(30);
    }

    // Update the corresponding File structure for the file being processed
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (strcmp(fileStats[i].filename, chunk->filename) == 0)
        {
            printf("Thread %lu counted %d words for chunk for file: %s, start: %d, end: %d\n", threadID, wordCount, chunk->filename, chunk->start, chunk->end);
            printf("Thread %lu counted %d words with consonants for chunk for file: %s, start: %d, end: %d\n", threadID, matchWords, chunk->filename, chunk->start, chunk->end);
            updateFileStats(chunk->filename, wordCount, matchWords, threadID);
            break;
        }
    }
}
