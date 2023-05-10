/**
 * @file main.c
 * @authors Pedro Sobral, Ricardo Rodriguez
 * @brief Development and validation of a multiprocess message passing application using the MPI library.
 *  
 * Problem: Text processing in Portuguese with Multithreading
 * 
 * The goal of this program is to receive command line arguments, which capture the names of the input files 
 * and the size of the chunk in bytes (optional) and process them in order to:
 * - get the number of words in the file;
 * - get the number of words with each vowel (['a','e','i','o','u','y']) in the file;
 * 
 * This program uses a multiprocess solution, using the MPI library, where the dispatcher (root process) will
 * read the chunk from the input files and send it to available worker processes (different from the root process).
 * 
 * The workers should process the chunk to extract the information (numWords and nWordsWithVowel) and then send the
 * partial results to the dispatcher, which will save all the partial results from the workers in order to get the
 * total numWords and nWordsWithVowel of each file.
 *
 * Dispatcher process workflow:
 *
 * 1 - Read and process the command line arguments;
 * 2 - Store filenames and initialize the structure related each file
 * 3 - Broadcast a message with the limit of bytes each chunk will have
 * 4 - While there's work to do / files to process (workStatus == 0):
 *      4.1 - Get a chunk with CHUNK_BYTE_LIMIT bytes of the current file we're analyzing
 *      4.2 - Send the chunk (and other important info) to the worker process for processing
 *      4.3 - Receive the partial results from the worker's processed chunk
 *      4.4 - Add the chunk results to the file results
 * 5 - Inform workers that all files are process and they can exit
 * 6 - Print the results of the text processing of all the input files
 * 7 - Finalize
 * 
 * Worker process workflow:
 *
 * 1 - Receive the broadcasted message from the dispatcher with the limit of bytes each chunk will have
 * 2 - While the dispatcher says there's work to be done (while workStatus == 0):
 *      2.1 - Receive the chunk and other important info from the dispatcher
 *      2.2 - Process the received chunk
 *      2.3 - Send the partial results from chunk processing to the dispatcher process
 * 3 - Finalize
 *
 */

#include <mpi.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "constants.h"
#include "utils.h"

/* Number of files to be processed */
int numFiles = 0;

/* Max. number of bytes allowed for a chunk */
int CHUNK_BYTE_LIMIT;

/* Stores the index of the current file being proccessed by the working processes */
int currentFileIndex = 0;

/* File structure declaration - will be used to store file related data (numWords, etc..) */
extern struct fileInfo *files;

/** Process control variable - used to know if there's still work to be done (or not) 
 * 0 -> Work is not done yet. There are still chunks/files that need to be read and/or processed
 * (!= 0) -> If workStatus = N, then process N received the last chunk and needs to process it, while others processes dont ask for another job/chunk
 */
int workStatus = 0;

/* Declaration of the function usage -> Usage of the program */
void usage();

/**
 * @brief Main program
 *
 * Dispatcher process workflow:
 *
 * 1 - Read and process the command line arguments;
 * 2 - Store filenames and initialize the structure related each file
 * 3 - Broadcast a message with the limit of bytes each chunk will have
 * 4 - While there's work to do / files to process (workStatus == 0):
 *      4.1 - Get a chunk with CHUNK_BYTE_LIMIT bytes of the current file we're analyzing
 *      4.2 - Send the chunk (and other important info) to the worker process for processing
 *      4.3 - Receive the partial results from the worker's processed chunk
 *      4.4 - Add the chunk results to the file results
 * 5 - Inform workers that all files are process and they can exit
 * 6 - Print the results of the text processing of all the input files
 * 7 - Finalize
 * 
 * Worker process workflow:
 *
 * 1 - Receive the broadcasted message from the dispatcher with the limit of bytes each chunk will have
 * 2 - While the dispatcher says there's work to be done (while workStatus == 0):
 *      2.1 - Receive the chunk and other important info from the dispatcher
 *      2.2 - Process the received chunk
 *      2.3 - Send the partial results from chunk processing to the dispatcher process
 * 3 - Finalize
 *
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char *argv[]) {

    /* Initialize MPI variables */
    int rank, size;     /* Rank - Process ID | Size - Number of processes (including root)*/
    unsigned int numWords = 0;          /* Variable used to store the number of words of a worker's processed chunk */
    unsigned int nWordsWithVowel[6];    /* Variable used to store the number of words with vowels [aeiouy] of a worker's processed chunk */
    unsigned int fileIndex;             /* Variable used to store the file index of a worker's processed chunk */
    
    /* Initialize the MPI communicator and get the rank of processes and the count of processes */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* ERROR. Number of processes should be limited. */
    if (size < 2 || size > 9) {
        fprintf(stderr, "Invalid number of processes (must be >= 2 and <= 9)");
        return EXIT_FAILURE;
    }

    /* ERROR. No arguments for filenames were introduced to the program. */
    if (argc < 2) {
        printf("[ERROR] Invalid number of files");
        MPI_Finalize();
        return 1;
    }

    const char *optstr = "f:c:h";           /* Acceptable command line arguments and parsing */
    int option;                             /* Store current command line arg */
    char *filenames[MAX_NUM_FILES];         /* Declare filenames array */
    CHUNK_BYTE_LIMIT = 4096;                /* Default chunk limit (in bytes) */

    if (rank == 0) {

        /* Structure used to keep track of the execution time */
        struct timespec start, finish;
        
        /* Clock start */
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);

        /**
        * Root process is the dispatcher. Thus, it's responsible of:
        * - processing the command line arguments
        * - store all the file's names 
        * - broadcast a message stating that he's ready to dispatch work for workers
        * - assign work to workers (with a corresponding buffer for processing)
        * - receive partial results from workers
        * - present the final results
        */

        /* Process command line arguments */
        while ((option = getopt(argc, argv, optstr)) != -1) {
            switch (option) {
                case 'f':
                    /* Save input files names*/
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
                    /* Define chunk size */
                    if (atoi(optarg) != 4096 && atoi(optarg) != 8192) {
                        fprintf(stderr, "Invalid chunk size (must be 4k or 8k)");
                        return EXIT_FAILURE;
                    }
                    CHUNK_BYTE_LIMIT = atoi(optarg);
                    break;
                case 'h':
                    /* Program usage */
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
        storeFilenames(files, filenames);

        /* Keep track of the current worker's rank ID */
        int nWorkers = 0;

		/* Broadcast message to working processes, so that they can start asking for chunks */
		MPI_Bcast(&CHUNK_BYTE_LIMIT, 1, MPI_INT, 0, MPI_COMM_WORLD);

        /* Wait for chunk requests while there are still files to be processed */
        while (!workStatus)
        {

            /* For all the worker processes */
            for (nWorkers = 1; nWorkers < size; nWorkers++)
            {
                /* All files were processed */
                if (workStatus != 0) {
                    break;
                }

                /* Allocate memory to support a fileChunk structure */
                struct fileChunk *chunkData = (struct fileChunk *) malloc(sizeof(struct fileChunk));

                /*Allocate memory for the chunk */
                chunkData->chunk = (unsigned char *) malloc(CHUNK_BYTE_LIMIT * sizeof(unsigned char));
                chunkData->isFinished = false;
                /* Get chunk data */
                chunkData->chunkSize = getChunk(chunkData, nWorkers);

                /* Send the workStatus, chunk, fileIndex and chunkSize to the worker process */
                MPI_Send(&workStatus, 1, MPI_INT, nWorkers, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD);
                MPI_Send(chunkData->chunk, CHUNK_BYTE_LIMIT, MPI_UNSIGNED_CHAR, nWorkers, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD);   /* the chunk buffer */
                MPI_Send(&chunkData->fileIndex, 1, MPI_UNSIGNED, nWorkers, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD);                  /* the index file of the chunk */
                MPI_Send(&chunkData->chunkSize, 1, MPI_UNSIGNED, nWorkers, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD);                  /* the size of the chunk */

                memset(chunkData->chunk, 0, CHUNK_BYTE_LIMIT * sizeof(unsigned char));

    
            }

            /* For all the worker processes */
            for (int i = 1; i < nWorkers; i++)
            {

                /* Receive the processing results from each worker process */
                MPI_Recv(&numWords, 1, MPI_UNSIGNED, i, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&nWordsWithVowel, 6, MPI_UNSIGNED, i, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&fileIndex, 1, MPI_UNSIGNED, i, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

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

    }
    else
    {

        /* Receive brodcast message from the dispatcher */
        MPI_Bcast(&CHUNK_BYTE_LIMIT, 1, MPI_INT, 0, MPI_COMM_WORLD);

        /* Allocate memory to support a fileChunk structure */
        struct fileChunk *chunkData = (struct fileChunk *) malloc(sizeof(struct fileChunk));
        chunkData->fileIndex = currentFileIndex;
        chunkData->chunk = (unsigned char *) malloc(CHUNK_BYTE_LIMIT * sizeof(unsigned char));
        chunkData->chunkSize = 0;
        chunkData->numWords = 0;
        for (int i = 0; i < 6; i++) {
            chunkData->nWordsWithVowel[i] = 0;
        }
        chunkData->isFinished = false;
        

        while (true)
        {
    
            /* Receive the control variable from the dispatcher to know if there's work still to be done (or not) */
            MPI_Recv(&workStatus, 1, MPI_UNSIGNED, 0, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            /* End worker if all files have been processed */
            if (workStatus != 0 && workStatus != rank) {
                break;
            }

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
            if (workStatus != 0) {
                break;
            }
            
        }

    }

    MPI_Finalize();
    exit(EXIT_SUCCESS);


}

/**
 * @brief prints the usage of the program
 *
 */
void usage() {
    printf("Usage:\n\t./prob1 -t <num_threads> -f <file1> <file2> ... <fileN> -c <chunk_size>\n\n");
    printf("\t-n <num_processes> : Number of processes to be used (1-8)\n");
    printf("\t-f <file1> <file2> ... <fileN> : List of files to be processed\n");
    printf("\t-c <chunk_size> : Chunk size (4k or 8k)\n");
}