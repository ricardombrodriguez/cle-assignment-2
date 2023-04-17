/**
 * @file main.c
 * @authors Pedro Sobral, Ricardo Rodriguez
 * @brief Text processing in Portuguese with Multithreading
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
#include "utils.h"
#include "sharedRegion.h"

/* Number of files to be processed */
int numFiles = 0;

int CHUNK_BYTE_LIMIT;

/* Number of threads */
int numThreads;

/* Stores working thread status */
int *threadStatus;

bool filesFinished = false;

void usage();

void *worker(void *id);


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

    const char *optstr = "t:f:c:h";
    int option;
    char *filenames[MAX_NUM_FILES];
    CHUNK_BYTE_LIMIT = 4096;

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
            case 'c':
                if (atoi(optarg) != 4096 && atoi(optarg) != 8192) {
                    fprintf(stderr, "Invalid chunk size (must be 4k or 8k)");
                    return EXIT_FAILURE;
                }
                CHUNK_BYTE_LIMIT = atoi(optarg);
                break;
            case 'h':
                usage();
                return EXIT_SUCCESS;
            default:
                fprintf(stderr, "Option Not Defined\n");
                return EXIT_FAILURE;
        }
    }

    printf("\n====== RUNNING PROGRAM FOR %d THREADS ======\n\n", numThreads);

    /* Initialize structures (setup) */
    storeFilenames(filenames);

    /* Array of pthreads */
    pthread_t threads[numThreads];

    /* Array of pthread status memory allocation */
    threadStatus = malloc(numThreads * sizeof(int));

    /* Pointer to the thread status */
    int *tStatus;

    unsigned int threadIDs[numThreads];

    // Creation of numThreads threads
    for (unsigned int i = 0; i < numThreads; i++) {

        threadIDs[i] = i;

        /* Thread worker creation */
        if (pthread_create(&threads[i], NULL, worker, &threadIDs[i]) != 0) {
            perror("[ERROR] Cannot create thread.\n");
            exit(1);
        }

    }

    /* waiting for the termination of the worker threads */

    for (int i = 0; i < numThreads; i++) {

        if (pthread_join(threads[i], (void *)&tStatus) != 0) {
            perror("[ERROR] Could not wait for worker thread.\n");
            exit(1);
        }

    }

    /* Present results */
    getResults();

    printf("\n====== PROGRAM EXECUTED SUCCESSFULLY FOR %d THREADS ======\n\n", numThreads);
    
    resetFilesData();

    end = clock();
    printf("Time elapsed: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    exit(EXIT_SUCCESS);
}

/**
 * @brief 
 * 
 * @param threadID 
 * @return void* 
 */
void *worker(void *id) {

    unsigned int threadID = *((unsigned int *) id);
    
    /* Allocate memory to support a fileChunk structure */
    struct fileChunk *chunkData = (struct fileChunk *)malloc(sizeof(struct fileChunk));
    chunkData->chunk = (unsigned char *) malloc(CHUNK_BYTE_LIMIT * sizeof(unsigned char));
    chunkData->isFinished = false;

    /* Keep running while there is tasks/work to do */
    while (true) {

        /* No more chunks to process, end worker function */
        if (filesFinished) {
            break;
        } 

        /* Get chunk data from shared region for latter process */
        getChunk(chunkData, threadID);

        /* After getting the chunk, process it */
        processChunk(chunkData);

        /* Store the results for the processed chunk by the working thread */
        saveThreadResults(chunkData, threadID);
    
        /* Initialize/reset chunk structure data */
        resetChunkData(chunkData);

    }

    /* Deallocate memory */
    free(chunkData);

    threadStatus[threadID] = 0;                 /* Success */
    void* tStatus = &threadStatus[threadID];
    pthread_exit(tStatus);
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