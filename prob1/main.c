/**
 * @file main.c
 * @authors Pedro Sobral, Ricardo Rodriguez
 * @brief Text processing in Portuguese with Multithreading
 *
 */

#include <mpi.h>
#include <stdbool.h>
#include <stdio.h>
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

/* Boolean control variable to see if all files have been processed */
bool filesFinished = false;

/* Stores the index of the current file being proccessed by the working processes */
int currentFileIndex = 0;

/* Process control variable - used to know if there's still work to be done (or not) */
int workStatus = 0;

extern struct fileInfo *files;

/* Declaration of the function usage -> Usage of the program */
void usage();

/**
 * @brief Main function
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char *argv[]) {

    /* Register start time of the clock */
    //clock_t start, end;
    //start = clock();

    printf("Bom dia matosinhos\n");

    /* Initialize MPI variables */
    int rank, size;     /* Rank - Process ID | Size - Number of processes (including root)*/
    int broadcast;      /* Message to be sent in the broadcast message */
    unsigned int numWords = 0;
    unsigned int nWordsWithVowel[6];
    

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
        MPI_Finalize();
        return 1;
    }

    const char *optstr = "f:c:h";           /* Acceptable command line arguments and parsing */
    int option;                             /* Store current command line arg */
    char *filenames[MAX_NUM_FILES];         /* Declare filenames array */
    CHUNK_BYTE_LIMIT = 4096;                /* Default chunk limit (in bytes) */

    if (rank == 0) {

        printf("rank 0 \n");

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

        files = (struct fileInfo *)malloc(numFiles * sizeof(struct fileInfo));
        /* Initialize fileInfo structure (setup and store filenames) */
        storeFilenames(files, filenames);

        printf("stored filenames\n");

        int nWorkers = 0;
        //workStatus = FILES_TO_BE_PROCESSED; // 1

		/* Broadcast message to working processes, so that they can start asking for chunks */
        broadcast = 1;
		MPI_Bcast(&broadcast, 1, MPI_INT, 0, MPI_COMM_WORLD);

        /* Wait for chunk requests while there are still files to be processed (workStatus = FILES_TO_BE_PROCESSED) */
        while (!workStatus)
        {

            for (nWorkers = 1; nWorkers < size; nWorkers++)
            {

                /* All files were processed */
                if (workStatus != 0) {
                    break;
                }

                /* Allocate memory to support a fileChunk structure */
                struct fileChunk *chunkData = (struct fileChunk *) malloc(sizeof(struct fileChunk));

                chunkData->chunk = (unsigned char *) malloc(CHUNK_BYTE_LIMIT * sizeof(unsigned char));
                chunkData->isFinished = false;

                /* Get chunk data */
                chunkData->chunkSize = getChunk(chunkData, nWorkers);

                MPI_Send(&workStatus, 1, MPI_INT, nWorkers, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD);
                MPI_Send(chunkData->chunk, CHUNK_BYTE_LIMIT, MPI_UNSIGNED_CHAR, nWorkers, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD);   /* the chunk buffer */
                MPI_Send(&chunkData->fileIndex, 1, MPI_UNSIGNED, nWorkers, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD);                  /* the index file of the chunk */
                MPI_Send(&chunkData->chunkSize, 1, MPI_UNSIGNED, nWorkers, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD);                  /* the size of the chunk */

                printf("[root] sent to process %d\n", nWorkers);

                memset(chunkData->chunk, 0, CHUNK_BYTE_LIMIT * sizeof(unsigned char));

    
            }

            for (int i = 1; i < nWorkers; i++)
            {
                unsigned int fileIndex;

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

        printf("======= ACABOU =============\n\n\n\n");

        //workStatus = ALL_FILES_PROCESSED;
		/* inform workers that all files are process and they can exit */
		for (int i = 1; i < size; i++) {
            printf("[ROOT] Sending end message to process %d\n", i);
		    MPI_Send(&workStatus, 1, MPI_INT, i, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD);
        }

		/* print the results of the text processing */
		getResults();

		/* calculate the elapsed time */
		//printf("\nElapsed time = %.6f s\n", (finish.tv_sec - start.tv_sec) / 1.0 + (finish.tv_nsec - start.tv_nsec) / 1000000000.0);

    }
    else
    {

        /* Receive brodcast message */
        MPI_Bcast(&broadcast, 1, MPI_INT, 0, MPI_COMM_WORLD);

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
            
            printf("Rank is %d \n", rank);
    
            /* Receive the control variable from the dispatcher to know if there's work still to be done (or not) */
            MPI_Recv(&workStatus, 1, MPI_UNSIGNED, 0, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            /* End worker if all files have been processed */
            if (workStatus != 0 && workStatus != rank) {
                printf("[RANK %d] LEFTTT\n", rank);
                break;
            }

            printf("[RANK %d] workStatus %u\n", rank, workStatus);

            /* Receive the chunk and other additional information for later processing from the dispatcher (root 0 process) */
            MPI_Recv(chunkData->chunk, CHUNK_BYTE_LIMIT, MPI_UNSIGNED_CHAR, 0, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&chunkData->fileIndex, 1, MPI_UNSIGNED, 0, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&chunkData->chunkSize, 1, MPI_UNSIGNED, 0, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            printf("[RANK %d] receivingggggggg..\n", rank);

            /* Process the received chunk */
            processChunk(chunkData);

            /* Send the partial results to the dispatcher (root 0 process) */
            MPI_Send(&chunkData->numWords, 1, MPI_UNSIGNED, 0, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD);
            MPI_Send(&chunkData->nWordsWithVowel, 6, MPI_UNSIGNED, 0, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD);
            MPI_Send(&chunkData->fileIndex, 1, MPI_UNSIGNED, 0, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD);

            printf("[RANK %d] sending %u words..\n", rank, chunkData->numWords);

            /* Reset chunk data */
            resetChunkData(chunkData);

            /* End worker if all files have been processed */
            if (workStatus != 0) {
                printf("[RANK %d] LEFTTT\n", rank);
                break;
            }
            
        }

    }

    printf("[RANK %d] FINALIZEEEE\n", rank);

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