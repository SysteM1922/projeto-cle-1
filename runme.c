#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

#define MAX_THREADS 8
#define DEFAULT_THREAD_SIZE 4000
#define MAX_FILES 20 // Maximum number of files


typedef struct {
    char* filename;           
    int fileSize;             // Size of the file - do i need this?
    int wordsCount;           // Total number of words counted
    int wordsWithConsonants; // Number of words with at least 2 consonants
    int isFinished;           // Flag indicating if the file processing is finished
} File;

File fileStats[MAX_FILES];
pthread_mutex_t fileStatsMutex = PTHREAD_MUTEX_INITIALIZER;


typedef struct {
    char* filename;  // Filename associated with the chunk
    int start;       // Start position of chunk
    int end;         // End position of chunk
} Chunk;

// Define structure for a node in the FIFO queue
typedef struct Node {
    Chunk* chunk;
    struct Node* next;
} Node;

// Define structure for FIFO queue
typedef struct {
    Node* front;
    Node* rear;
    pthread_mutex_t mutex;
} Queue;

// Define structure for thread-specific data
typedef struct {
    pthread_t threadID;             // Thread ID
    Queue* fifo;                    // Pointer to the FIFO queue
    File* fileStats;                // Pointer to file statistics
} ThreadData;

// Initialize FIFO queue
void initQueue(Queue* q) {
    q->front = NULL;
    q->rear = NULL;
    pthread_mutex_init(&q->mutex, NULL);
}

// Enqueue a chunk into the FIFO queue
void enqueue(Queue* q, Chunk* chunk) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->chunk = chunk;
    newNode->next = NULL;
    
    /* Make sure this is done in mutual exclusion!  */
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

    // Make sure this is done in mutual exclusion!
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


// Function to process a chunk
void processChunk(Chunk* chunk, pthread_t threadID) {

    int numWords = rand() % 1000;               // Random number of words
    int numWordsWithConsonants = rand() % 200; // Random number of words with consonants

    // insert code here that actually counts the words and words with consonants 
    // then update the fileStats structure
    // ...

    // Find the corresponding File structure for the file being processed
    // i think this might not be the way to go, but for now i will leave it
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(fileStats[i].filename, chunk->filename) == 0) {
            pthread_mutex_lock(&fileStatsMutex);
            fileStats[i].wordsCount += numWords;
            fileStats[i].wordsWithConsonants += numWordsWithConsonants;
            pthread_mutex_unlock(&fileStatsMutex);
            break;
        }
    }
    
    // Print the number of words counted by this thread for this chunk
    printf("Thread %lu counted %d words for chunk for file: %s, start: %d, end: %d\n", threadID, numWords, chunk->filename, chunk->start, chunk->end);
    printf("Thread %lu counted %d words with consonants for chunk for file: %s, start: %d, end: %d\n", threadID, numWordsWithConsonants, chunk->filename, chunk->start, chunk->end);

    // Simulate processing delay (200 milliseconds)
    usleep(200000); // 200 milliseconds = 200,000 microseconds
}

// Thread start routine
void* threadStartRoutine(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    Queue* fifo = data->fifo;
    pthread_t threadID = pthread_self(); // Get the thread ID
    
    while (1) {
        Chunk* chunk = dequeue(fifo);   // get the next chunk from the queue
        if (chunk == NULL) {
            // No more work, thread exits
            break;
        }
        processChunk(chunk, threadID); // Pass thread ID to the process function 
        free(chunk->filename);
        free(chunk);
    }
    
    pthread_exit(NULL);
}

static void printFileStats(int totalWords, int wordsWithConsonants);

static double get_delta_time(void);

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

            case -1:  break;
        }
    }
    
    // Store filenames
    for (int i = optind; i < argc; i++) {
        // printf("Filename: %s\n", argv[i]);
        filenames[numFiles++] = strdup(argv[i]);
    }
    
    // Ensure that filenames are provided
    if (numFiles == 0) {
        printf("Usage: %s -t <num_threads> -s <size_thread> <filename1> <filename2> ...\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    get_delta_time();

    // Initialize the File structures for each file
    for (int i = 0; i < numFiles; i++) {
        fileStats[i].filename = filenames[i];
        fileStats[i].fileSize = 0; // This will be updated later
        fileStats[i].wordsCount = 0;
        fileStats[i].wordsWithConsonants = 0;
        fileStats[i].isFinished = 0;
    }

    // Initialize FIFO queue
    Queue fifo;
    initQueue(&fifo);
    
    // Determine file sizes and enqueue chunks
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
                enqueue(&fifo, chunk);
                start = end + 1;
            }
        } else {
            perror("Error determining file size");
            exit(EXIT_FAILURE);
        }
    }
    
    // Create threads and process chunks
    pthread_t threads[numThreads];
    ThreadData threadData[numThreads];
    for (int i = 0; i < numThreads; i++) {
        threadData[i].fifo = &fifo;
        pthread_create(&threads[i], NULL, threadStartRoutine, (void*)&threadData[i]);
    }
    
        // Join threads and perform cleanup
    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Print statistics for each file
    printf("\n-----------------------------File Statistics-----------------------------\n");
    for (int i = 0; i < numFiles; i++) {
        printf("File: %s\n", fileStats[i].filename);
        printFileStats(fileStats[i].wordsCount, fileStats[i].wordsWithConsonants);
    }
    
    // Cleanup
    for (int i = 0; i < numFiles; i++) {
        free(filenames[i]);
    }

    printf("Elapsed Time: %f\n", get_delta_time());
    
    return 0;
}

static void printFileStats(int totalWords, int wordsWithConsonants) {
    printf("Total number of words counted: %d\n", totalWords);
    printf("Number of words with at least 2 consonants: %d\n", wordsWithConsonants);
    printf("\n");
}


static double get_delta_time(void)
{
    static struct timespec t0, t1;
    t0 = t1;
    if (clock_gettime(CLOCK_MONOTONIC, &t1) != 0)
    {
        perror("clock_gettime");
        exit(1);
    }
    return (double)(t1.tv_sec - t0.tv_sec) + 1.0e-9 * (double)(t1.tv_nsec - t0.tv_nsec);
}
