/**
 * \file main.c
 * 
 * \brief Main file of the program
 * 
 * This file contains the main function of the program. It reads command-line arguments, initializes file structures,
 * 
 * \author Guilherme Antunes - 103600
 * \author Pedro Rasinhas - 103541
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>

#include "sharedRegion.h"
#include "workerThreads.h"
#include "args.h"

#define MAX_THREADS 8
#define DEFAULT_THREAD_SIZE 4096
#define MAX_FILES 20 

static double get_delta_time(void);

/**
 * @brief Main function
 * 
 * The main function of the program. Reads command-line arguments, initializes file structures, 
 * and starts worker threads to process file chunks.
 * 
 * @param argc Number of command-line arguments
 * @param argv Array of command-line arguments
 * @return Exit status
 */
int main(int argc, char* argv[]) {
    int numThreads = 4; // Default number of threads
    int threadSize = DEFAULT_THREAD_SIZE; // Default size of thread's chunk (4000 bytes)
    
    char* filenames[MAX_FILES]; // Array to store filenames
    int numFiles = 0; // Number of filenames
    
    // Parse command-line arguments
    int opt;
    while ((opt = getopt(argc, argv, "t:s:")) != -1) {
        switch (opt) {
            case 't':
                numThreads = atoi(optarg);
                if (numThreads <= 0 || numThreads > MAX_THREADS) {
                    printf("Number of threads must be between 1 and %d\n", MAX_THREADS);
                    exit(EXIT_FAILURE);
                }
                break;
            case 's':
                threadSize = atoi(optarg);
                if (threadSize <= 0) {
                    printf("Thread size must be a positive integer\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'h':
                printf("Usage: %s -t <num_threads> -s <size_thread> <filename1> <filename2> ...\n", argv[0]);
                exit(EXIT_SUCCESS);
            case '?':
                printf("invalid option\n");
                printf("Usage: %s -t <num_threads> -s <size_thread> <filename1> <filename2> ...\n", argv[0]);
                exit(EXIT_FAILURE);
            case -1: break;
        }
    }
    
    // Store filenames
    for (int i = optind; i < argc; i++) {
        filenames[numFiles++] = strdup(argv[i]);
    }
    
    // Ensure that filenames are provided
    if (numFiles == 0) {
        printf("Usage: %s -t <num_threads> -s <size_thread> <filename1> <filename2> ...\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    get_delta_time();

    // Initialize the File structures for each file
    initFileStructures(filenames, numFiles);

    // Initialize FIFO queue
    Queue fifo;
    initQueue(&fifo);
    
    // Determine file sizes and enqueue chunks
    enqueueChunks(&fifo, filenames, numFiles, threadSize);
    
    // Create threads and process chunks
    pthread_t threads[numThreads];
    ThreadData threadData[numThreads];
    createThreads(numThreads, &fifo, threads, threadData);
    // Join threads and perform cleanup
    joinThreads(numThreads, threads);   // Wait for all threads to finish

    // Print statistics for each file
    printf("\n-----------------------------File Statistics-----------------------------\n");
    for (int i = 0; i < numFiles; i++) {
        printf("File: %s\n", fileStats[i].filename);
        printf("Total number of words: %d\n", fileStats[i].wordsCount);
        printf("Total number of words with at least two instances of the same consonant: %d\n", fileStats[i].wordsWithConsonants);
        printf("\n");
    }
    printf("Elapsed Time: %fs\n ", get_delta_time());
    // Cleanup
    for (int i = 0; i < numFiles; i++) {
        free(filenames[i]);
    }
    return 0;
}

/**
 * @brief Get the process time that has elapsed since last call of this time.
 * 
 * @return Process elapsed time
 */
static double get_delta_time(void)
{
    static struct timespec t0, t1;

    t0 = t1;
    if(clock_gettime (CLOCK_MONOTONIC, &t1) != 0) {
        perror("clock_gettime");
        exit(1);
    }
    return (double) (t1.tv_sec - t0.tv_sec) + 1.0e-9 * (double) (t1.tv_nsec - t0.tv_nsec);
}
