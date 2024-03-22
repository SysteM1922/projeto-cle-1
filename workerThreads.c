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

int isLetter(wchar_t c) {
    return iswalnum(c) || c == L'_';
}

int isConsonant(wchar_t c) {
    return wcschr(L"bcdfghjklmnpqrstvwxyz", c) != NULL;
}

void extractLetter(wchar_t *letter) {
    *letter = towlower(*letter);
    wchar_t *simpleLetters = L"c";
    wchar_t *complexLetters = L"ç";
 
    for (int i = 0; i < wcslen(complexLetters); i++) {
        if (complexLetters[i] == *letter) {
            *letter = simpleLetters[i];
            break;
        }
    }
}

void createThreads(int numThreads, Queue *fifo, pthread_t *threads, ThreadData *threadData)
{
    for (int i = 0; i < numThreads; i++)
    {
        threadData[i].fifo = fifo;
        pthread_create(&threads[i], NULL, threadStartRoutine, (void *)&threadData[i]);
    }
}

void joinThreads(int numThreads, pthread_t *threads)
{
    for (int i = 0; i < numThreads; i++)
    {
        pthread_join(threads[i], NULL);
    }
}

// Thread start routine
void *threadStartRoutine(void *arg)
{
    ThreadData *data = (ThreadData *)arg;
    Queue *fifo = data->fifo;

    setlocale(LC_ALL, "");

    while (1)
    {
        Chunk *chunk = dequeue(fifo);
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
        processChunk(chunk, pthread_self());

        // Update the next chunk's start position ONLY if the previous chunk ended in the middle of a word!!!
        // Check if the chunk ended in the middle of a word
        if (chunk->end < ftell(file)) {
            // Adjust the chunk's end position to the end of the word
            chunk->end = ftell(file);

            // Update the next chunk's starting position
            Chunk* nextChunk = dequeue(fifo);
            
            if (nextChunk != NULL) {
                nextChunk->start = chunk->end;
                enqueue(fifo, nextChunk);
            }
        }


    }
    return NULL;
}

// Function to process a chunk
void processChunk(Chunk *chunk, pthread_t threadID) {
    FILE *file = fopen(chunk->filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    // Seek to the start of the chunk
    fseek(file, chunk->start, SEEK_SET);

    int wordCount = 0;
    int inWord = 0;
    int matchWords = 0;
    wchar_t word[21];
    int word_len = 0;
    wchar_t c;
    int found = 0;
    while ((c = fgetwc(file)) != WEOF) {
        if (isLetter(c)) {
            if (found) {
                continue;
            } else {
                extractLetter(&c);
                if (!inWord) {
                    wordCount++;
                    inWord = 1;
                }
                if (isConsonant(c)) {
                    for (int i = 0; i < word_len; i++) {
                        if (word[i] == c) {
                            found = 1;
                            matchWords++;
                            break;
                        }
                    }
                    if (!found) {
                        word[word_len] = c;
                        word_len++;
                    }
                }
            }
        } else if (c != L'’' && c != L'‘' && c != L'\'' ) {
            inWord = 0;
            found = 0;
            memset(word, '\0', sizeof(word));
            word_len = 0;
        }

        // Check if the end of the chunk is reached
        if (ftell(file) >= chunk->end) {
            // If in the middle of a word, adjust the chunk's end position
            if (inWord) {
                // printf("Thread %lu reached the end of the chunk in the middle of a word for file: %s, start: %d, end: %d\n", threadID, chunk->filename, chunk->start, chunk->end);
                // Keep reading byte by byte until a whitespace or separation character is found
                while ((c = fgetwc(file)) != WEOF && (isLetter(c) || c == L'’' || c == L'‘' || c == L'\'')) {
                    // printf("Current character: %lc\n", c);
                    // printf("Thread %lu continued reading after reaching the end of the chunk for file: %s, start: %d, end: %d\n", threadID, chunk->filename, chunk->start, chunk->end);
                    // printf("The end of the chunk was: %d\n", chunk->end);
                    // printf("The current position is: %ld\n", ftell(file));
                    // printf("\n\n");
                    continue;
                }
                // Update the chunk's end position once a whitespace or separation character is found
                chunk->end = ftell(file);
            }
            break; // Exit the loop when the end of the chunk is reached
        }
        
    }

    fclose(file);

    // Update the corresponding File structure for the file being processed
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(fileStats[i].filename, chunk->filename) == 0) {
            updateFileStats(i, wordCount, matchWords);
            break;
        }
    }

    // Print the number of words counted by this thread for this chunk
    // printf("Thread %lu counted %d words for chunk for file: %s, start: %d, end: %d\n", threadID, wordCount, chunk->filename, chunk->start, chunk->end);
    // printf("Thread %lu counted %d words with consonants for chunk for file: %s, start: %d, end: %d\n", threadID, matchWords, chunk->filename, chunk->start, chunk->end);

    // Simulate processing delay (200 milliseconds)
    // usleep(200000); // 200 milliseconds = 200,000 microseconds
}