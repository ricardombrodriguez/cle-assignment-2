
/**
 * @file main.c
 * @authors Pedro Sobral, Ricardo Rodriguez
 * @brief Text processing in Portuguese with Multithreading
 * @copyright Copyright (c) 2023
 * 
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include "constants.h"
#include "sharedRegion.h"
#include "utils.h"

/* Number of files to be processed */
unsigned int numFiles;

/* Number of threads */
int numThreads;

/* Stores working thread status */
int *threadStatus;

/* Boolean to check if files struct has been initialized (or not) */
bool initializedData = false;

/* Condition variable for synchronization */
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/* Mutex for synchronization */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* File sructure initialization */
extern struct file *files;

/* Stores the index of the current file being proccessed by the working threads */
extern int currentFileIndex;

/* Status of the files processing */
bool isFinished = false;

/* Is there an available job for working threads? */
bool jobAvailable = true;

extern int finishedWorkers;

extern bool threadFinished[];

extern bool requestedJob;

struct chunk sharedChunk;


void usage();

void *worker(void *id);

void *distributor();


/**
 * @brief 
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char *argv[]) {

    clock_t start, end;
    start = clock();
  
    /* ERROR. No arguments for filenames were introduced to the program. */
    if (argc < 2) {
        printf("[ERROR] Invalid number of files");
        return 1;
    }

    const char *optstr = "t:f:h";
    int option;
    char *filenames[MAX_NUM_FILES];

    while ((option = getopt(argc, argv, optstr)) != -1) {
        switch (option) {
            case 't':
                if (atoi(optarg) < 1 || atoi(optarg) > 8) {
                    fprintf(stderr, "Invalid number of threads (must be >= 1 and <= 8)");
                    return EXIT_FAILURE;
                }
                numThreads = atoi(optarg);
                break;
            case 'f':
                filenames[numFiles++] = optarg;
                while (optind < argc && argv[optind][0] != '-') {
                    if (access(argv[optind], F_OK) == 0) {
                        // file exists
                        filenames[numFiles++] = argv[optind];
                    } else {
                        // file doesn't exist, show error and exit program
                        printf("[ERROR] %s file doesn't exist", argv[optind]);
                        return 1;
                    }
                    optind++;
                }
                break;
            case 'h':
                usage();
                return EXIT_SUCCESS;
            default:
                fprintf(stderr, "Option Not Defined\n");
                return EXIT_FAILURE;
        }
    }

    //print filename
    for (int i = 0; i < numFiles; i++) {
        printf("Filename: %s\n", filenames[i]);
    }


    printf("\n====== RUNNING PROGRAM FOR %d THREADS ======\n\n", numThreads);

    numThreads++;           /* Consider distributer thread */

    /* Array of pthreads */
    pthread_t threads[numThreads];

    /* Array of pthread status memory allocation */
    threadStatus = malloc(numThreads * sizeof(int));

    /* Pointer to the thread status */
    int *tStatus;

    unsigned int threadIDs[numThreads];

    /* Thread worker creation */
    if (pthread_create(&threads[0], NULL, distributor, filenames) != 0) {
        perror("[ERROR] Cannot create thread.\n");
        exit(1);
    }

    pthread_mutex_lock(&mutex);
    while (!initializedData) {
        pthread_cond_wait(&cond, &mutex);
    }
    
    /* Unlock mutex to create threads */
    pthread_mutex_unlock(&mutex);

    printf("DATA INITIALIZED\n");

    // Creation of numThreads threads
    for (unsigned int i = 1; i < numThreads; i++) {

        printf("Working thread %u created\n", i);

        threadIDs[i] = i;

        /* Thread worker creation */
        if (pthread_create(&threads[i], NULL, worker, &threadIDs[i]) != 0) {
            perror("[ERROR] Cannot create thread.\n");
            exit(1);
        }
    }

    /* waiting for the termination of the worker threads */

    for (int i = 1; i < numThreads; i++) {

        printf("Working thread %u joined\n", i);

        if (pthread_join(threads[i], (void *)&tStatus) != 0) {
            perror("[ERROR] Could not wait for worker thread.\n");
            exit(1);
        }

    }

    /* Present results */
    
    printf("\n====== PROGRAM EXECUTED SUCCESSFULLY FOR %d THREADS ======\n\n", numThreads);
    
    end = clock();
    printf("Time elapsed: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    exit(EXIT_SUCCESS);
}




/**
 * @brief 
 * 
 * @param threadID 
 * @return void
 */
void *worker(void *id) {

    unsigned int threadID = *((unsigned int *) id);

    while (true) {

        /* Mutex lock */
        if ((pthread_mutex_lock(&mutex)) != 0) {
            printf("[ERROR] Can't enter the mutex critical zone.");
            pthread_exit(0);
        }

        /* Add to queue */
        threadQueue[currentQueueIndex] = threadID;
        currentQueueIndex++;
        requestedJob = true;


        if ((pthread_cond_wait(&cond, &mutex)) != 0) {
            printf("[ERROR] pthread_cond_wait.");
            pthread_exit(0);
        }

        if ((pthread_mutex_unlock(&mutex)) != 0) {
            printf("[ERROR] Can't leave the mutex critical zone.");
            pthread_exit(0);
        }

        struct chunk chunkData = sharedChunk;
        
        // process the chunk of data
        processChunk(chunkData, threadID);

        saveThreadResults(chunkData, threadID);

        resetChunkData(chunkData, threadID);

        /* Notify distributor */

        /* Mutex lock */
        if ((pthread_mutex_lock(&mutex)) != 0) {
            printf("[ERROR] Can't enter the mutex critical zone.");
            pthread_exit(0);
        }
    
        /* Deallocate memory */
        free(chunkData);

        finishedWorkers++;

        if ((pthread_cond_signal(&cond, &mutex)) != 0) {
            printf("[ERROR] pthread_cond_signal.");
            pthread_exit(0);
        }

        if ((pthread_mutex_unlock(&mutex)) != 0) {
            printf("[ERROR] Can't leave the mutex critical zone.");
            pthread_exit(0);
        }

        void* tStatus = &threadStatus[threadID];
        pthread_exit(tStatus);
        
    }

}



// Distributor thread function
void *distributor(char *filenames[]) {

    printf("no distribuidor\n");
    
    int i;
    FILE *file;
    unsigned int numNumbers[numThreads];

    for (i = 0; i < numFiles; i++) {

        // Read the sequence of integers from the binary file
        file = fopen(filenames[i], "rb");
        if (!file) {
            perror("Error opening file");
            exit(1);
        }

        int numberOfValues = 0;
        fread(&numberOfValues, sizeof(int), 1, file);

        numNumbers[i] = numberOfValues;

        printf("Number of values: %d\n", numberOfValues);

        fclose(file);

    }

    /* Initialize files structure */
    storeFilenames(filenames, numNumbers);

    /* Set initializedData to true so the main.c can create the working threads */
    if ((pthread_mutex_lock(&mutex)) != 0) {
        printf("[ERROR] Can't enter the mutex critical zone.");
        pthread_exit(0);
    }

    initializedData = true;

    if ((pthread_cond_signal(&cond)) != 0) {
        printf("[ERROR] pthread_cond_signal.");
        pthread_exit(0);
    }
    
    if ((pthread_mutex_unlock(&mutex)) != 0) {
        printf("[ERROR] Can't leave the mutex critical zone.");
        pthread_exit(0);
    }

    /* Wait for requests and assign work */
    while (finishedWorkers < numThreads) {

        /* Wait for thread request */
        while (!requestedJob) {

            if (threadQueue == empty) 
                break;

            if ((pthread_cond_wait(&request_cond, &mutex)) != 0) {
                printf("[ERROR] pthread_cond_wait.");
                pthread_exit(0);
            }

        }

        /* Delegate job */

        int threadID = threadQueue[0];

        sharedChunk = getChunk();


        /* Mutex unlock */
        if ((pthread_mutex_unlock(&mutex)) != 0) {
            printf("[ERROR] Can't leave the mutex critical zone.");
            pthread_exit(0);
        }
        
        // distribute the chunk to the workers
        for (int i = 0; i < NUM_WORKERS; i++) {
            
            // check if worker is finished processing its own chunk
            if (i == start_index / CHUNK_SIZE) {
                continue;
            }
            
            // send the chunk to the worker
            if ((pthread_mutex_lock(&mutex)) != 0) {
                printf("[ERROR] Can't enter the mutex critical zone.");
                pthread_exit(0);
            }

            memcpy(sharedChunk, &start_index, sizeof(start_index));

            if ((pthread_cond_signal(&cond)) != 0) {
                printf("[ERROR] pthread_cond_signal.");
                pthread_exit(0);
            }

            if ((pthread_mutex_unlock(&mutex)) != 0) {
                printf("[ERROR] Can't unlock the mutex critical zone.");
                pthread_exit(0);
            }


        }
        
        // wait for all workers to finish processing the chunk
        pthread_mutex_lock(&mutex);
        while (finished_workers < start_index / CHUNK_SIZE) {
            pthread_cond_wait(&cond, &mutex);
        }

        pthread_mutex_unlock(&mutex);

    }

}


/**
 * @brief prints the usage of the program
 * 
 */
void usage() {
    printf("Usage:\n\t./prob1 -t <num_threads> -f <file1> <file2> ... <fileN> -c <chunk_size>\n\n");
    printf("\t-t <num_threads> : Number of threads to be used (1-8)\n");
    printf("\t-f <file1> <file2> ... <fileN> : List of files to be processed\n");
    printf("\t-c <chunk_size> : Chunk size (4k or 8k)\n");
}

