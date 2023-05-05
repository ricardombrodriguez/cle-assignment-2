
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
#include <unistd.h>
#include <time.h>
#include <mpi.h>

#include "constants.h"
#include "utils.h"

/* Number of files to be processed */
unsigned int numFiles;

/* File sructure initialization */
extern struct file *files;

/* Stores the index of the current file being proccessed by the working threads */
extern int currentFileIndex;

int isFinished;



void usage();

/**
 * @brief 
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char *argv[]) {

    /* Initialize MPI variables */
    int rank, size;     /* Rank - Process ID | Size - Number of processes (including root)*/
    unsigned int *sequence;         /* Variable used to the array of integers of a received chunk */
    unsigned int size;              /* Variable used to store the number of integers of the chunk */
    int isSorted;                   /* Variable used to know if the chunk's integer array is sorted */
    unsigned int sequenceSize;

    /* Initialize the MPI communicator and get the rank of processes and the count of processes */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* ERROR. Number of processes should be limited. */
    if (size < 1 || size > 8) {
        fprintf(stderr, "Invalid number of processes (must be >= 1 and <= 8)");
        return EXIT_FAILURE;
    }
  
    /* ERROR. No arguments for filenames were introduced to the program. */
    if (argc < 2) {
        printf("[ERROR] Invalid number of files");
        return 1;
    }

    const char *optstr = "t:f:h";
    int option;
    char *filenames[MAX_NUM_FILES];

    if (rank == 0) {

        /* Structure used to keep track of the execution time */
        struct timespec start, finish;
        
        /* Clock start */
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);

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

        /* Allocation of memory to the fileInfo structure */
        files = (struct fileInfo *)malloc(numFiles * sizeof(struct fileInfo));

        /* Initialize fileInfo structure (setup and store filenames) */
        storeFilenames(files, filenames, size);

        /* Keep track of the current worker's rank ID */
        int nWorkers = 0;

        /* Wait for chunk requests while there are still files to be processed */
        while (!isFinished)
        {

            sequenceSize = (files + currentFileIndex)->numNumbers % (size - 1);

            /* Broadcast message to working processes, so that they can start asking for integer chunks */
            MPI_Bcast(&sequenceSize, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);

            /* For all the worker processes */
            for (nWorkers = 1; nWorkers < size; nWorkers++)
            {
                /* Allocate memory to support a Sequence structure */
                struct Sequence *sequence = (struct Sequence *) malloc(sizeof(struct Sequence));

                /*Allocate memory for the chunk */
                sequence->sequence = (unsigned int *) malloc(sequenceSize * sizeof(unsigned int));
                sequence->isSorted = 0;
                /* Get chunk data */
                
                sequence->size = sequenceSize;
                /* Last worker can have a different sequence size, since the sequences could not be splitted equally through all workers */
                if (nWorkers == size - 1) {
                    sequence->size = (files + currentFileIndex)->numNumbers - (chunkNumbers * (size - 2));
                }
            
                getChunk(sequence, nWorkers);

                MPI_Send(&isFinished, 1, MPI_INT, nWorkers, MPI_TAG_PROGRAM_STATE, MPI_COMM_WORLD);

                MPI_Send(sequence->sequence, chunkNumbers, MPI_UNSIGNED, nWorkers, MPI_TAG_CHUNK_REQUEST, MPI_COMM_WORLD);      /* the chunk buffer */
                MPI_Send(&sequence->size, 1, MPI_UNSIGNED, nWorkers, MPI_TAG_CHUNK_REQUEST, MPI_COMM_WORLD);                    /* the index file of the chunk */
                MPI_Send(&sequence->isSorted, 1, MPI_INT, nWorkers, MPI_TAG_CHUNK_REQUEST, MPI_COMM_WORLD);                     /* the size of the chunk */

                memset(sequence->chunk, 0, CHUNK_BYTE_LIMIT * sizeof(unsigned char));

    
            }

            /* For all the worker processes */
            for (int i = 1; i < nWorkers; i++)
            {

                /* Receive the processing results from each worker process */
                MPI_Recv(&sequence, chunkNumbers, MPI_UNSIGNED, i, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&sequenceSize, 6, MPI_UNSIGNED, i, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&isSorted, 1, MPI_INT, i, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                /* Update/store partial results from the processed chunk */
                (files + fileIndex)->numWords += numWords;
                for (int j = 0; j < 6; j++) {
                    (files + fileIndex)->nWordsWithVowel[j] += nWordsWithVowel[j];
                }

            }

        }

		/* Inform workers that all files are process and they can exit */
		for (int i = 1; i < size; i++) {
		    MPI_Send(&workStatus, 1, MPI_INT, i, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD);
        }

        /* Clock end */
        clock_gettime(CLOCK_MONOTONIC_RAW, &finish);

        /* Print the results of the text processing of all files */
        getResults();

        /* Calculate execution time */
        printf("\eExecution time = %.6f s\n", (finish.tv_sec - start.tv_sec) / 1.0 + (finish.tv_nsec - start.tv_nsec) / 1000000000.0);

    } else {


        /*
        
        workers
        
        
        
        */

        /* Receive brodcast message from the dispatcher */
        MPI_Bcast(&sequenceSize, 1, MPI_INT, 0, MPI_COMM_WORLD);

        /* Allocate memory to support a Sequence structure */
        struct Sequence *sequence = (struct Sequence *) malloc(sizeof(struct Sequence));
        /*Allocate memory for the chunk */
        sequence->sequence = (unsigned int *) malloc(sequenceSize * sizeof(unsigned int));
        sequence->isSorted = 0;
        sequence->size = 0;

        while (true)
        {
    
            /* Receive the control variable from the dispatcher to know if there's work still to be done (or not) */
            MPI_Recv(&isFinished, 1, MPI_INT, 0, MPI_TAG_PROGRAM_STATE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            /* End worker if all files have been processed */
            if (isFinished)
                break;
            
            /* Receive the chunk and other additional information for later processing from the dispatcher (root 0 process) */
            MPI_Recv(chunkData->chunk, CHUNK_BYTE_LIMIT, MPI_UNSIGNED_CHAR, 0, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&chunkData->fileIndex, 1, MPI_UNSIGNED, 0, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&chunkData->chunkSize, 1, MPI_UNSIGNED, 0, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            /* Process the received chunk */
            processChunk(chunkData);

            /* Send the partial results to the dispatcher (root 0 process) */
            MPI_Send(&chunkData->numWords, 1, MPI_UNSIGNED, 0, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD);
            MPI_Send(&chunkData->nWordsWithVowel, 6, MPI_UNSIGNED, 0, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD);
            MPI_Send(&chunkData->fileIndex, 1, MPI_UNSIGNED, 0, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD);

            /* Reset chunk data */
            resetChunkData(chunkData);

            /* End worker if all files have been processed */
            if (isFinished)
                break;
            
        }


    }

    MPI_Finalize();
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

