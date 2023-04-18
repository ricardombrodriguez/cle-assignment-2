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
#include "sharedRegion.h"
#include "utils.h"

/* Number of files to be processed */
int numFiles = 0;

/* Max. number of bytes allowed for a chunk */
int CHUNK_BYTE_LIMIT;

/* Number of threads */
int numThreads;

/* Stores working thread status */
int *threadStatus;

/* Boolean control variable to see if all files have been processed */
bool filesFinished = false;

void usage();

void *worker(void *id);

/**
 * @brief Main function
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char *argv[]) {

    /* Register start time of the clock */
    clock_t start, end;
    start = clock();

    /* Initialize MPI variables */
    int rank, size;
    int workStatus;

    /* Initialize the MPI communicator and get the rank of processes and the count of processes */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* ERROR. No arguments for filenames were introduced to the program. */
    if (argc < 2) {
        printf("[ERROR] Invalid number of files");
        MPI_Finalize();
        return 1;
    }

    const char *optstr = "t:f:c:h";         /* Acceptable command line arguments and parsing */
    int option;                             /* Store current command line arg */
    char *filenames[MAX_NUM_FILES];         /* Declare filenames array */
    CHUNK_BYTE_LIMIT = 4096;                

    if (rank == 0) {
        /**
        * Root process is the dispatcher. Thus, it's responsible of:
        * - processing the command line arguments
        * - store all the file's names 
        * - broadcast a message stating that he's ready to dispatch work for workers
        * - assign work to workers (with a corresponding buffer for processing)
        * - receive partial results from workers
        * - present the final results
        */

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

        MPI_Bcast(&CHUNK_BYTE_LIMIT, 1, MPI_INT, 0, MPI_COMM_WORLD);
        

        struct fileInfo *files = (struct fileInfo *)malloc(numFiles * sizeof(struct fileInfo));
        int nWorkers = 0;
        int previousChar = 0;
        int numWords = 0;
        workStatus = FILES_TO_BE_PROCESSED;

        usigned char *chunk = (unsigned char *)malloc(CHUNK_BYTE_LIMIT * sizeof(unsigned char));
        memset(chunk, 0, CHUNK_BYTE_LIMIT * sizeof(unsigned char));

        for (int i= 0; i < numFiles; i++)
        {
            (files + i)->filename = filenames[i];
            (files + i)->fp = NULL;

            if (((files + i)->fp = fopen(filenames[i], "rb")) == NULL) {
                printf("[ERROR] %s file doesn't exist\n", filenames[i]);
                exit(EXIT_FAILURE);
            }

            while (!((files + i)->isFinished))
            {
                for (nWorkers = 1; nWorkers < size; nWorkers++)
                {
                    if ((files + i)->isFinished)
                    {
                        fclose((files + i)->fp);
                        break;
                    }

                    previousChar = (files + i)->previousChar;
                    (files + i)->chunkSize =  fread(chunk, sizeof(unsigned char), CHUNK_BYTE_LIMIT, (files + i)->fp);
               
                    if ((files + i)->chunkSize < CHUNK_BYTE_LIMIT)
                    {
                        (files + i)->isFinished = true;
                    } else {
                        //getChunkSizeLastChar(chunk, files + i)
                    }

                    if ((files + i )->previousChar == EOF)
                    {
                        (files + i)->isFinished = true;
                    }

                    MPI_Send(&workStatus, 1, MPI_INT, nWorkers, 0, MPI_COMM_WORLD);
                    MPI_Send(chunk, maxBytesPerChunk, MPI_UNSIGNED_CHAR, nWorkers, 0, MPI_COMM_WORLD);/* the chunk buffer */
                    MPI_Send(&(filesData + nFile)->chunkSize, 1, MPI_INT, nWorkers, 0, MPI_COMM_WORLD);/* the size of the chunk */
                    MPI_Send(&previousCh, 1, MPI_INT, nWorkers, 0, MPI_COMM_WORLD);/* the character of the previous chunk */

                    memset(chunk, 0, CHUNK_BYTE_LIMIT * sizeof(unsigned char));
                }
                
                for (i = 1; i < nWorkers; i++)
                {
                /* Receive the processing results from each worker process */
                MPI_Recv(&numWords, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                //MPI_Recv(&nWordsBV, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                //MPI_Recv(&nWordsEC, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                /* update struct with new results */
                (files + i)->nWords += numWords;
                (files + i)->nWordsBV += nWordsBV;
                (files + i)->nWordsEC += nWordsEC;
                }
            }

            /* no more work to be done */
            workStatus = ALL_FILES_PROCESSED;
            /* inform workers that all files are process and they can exit */
            for (i = 1; i < size; i++)
            MPI_Send(&workStatus, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

            /* timer ends */
            clock_gettime(CLOCK_MONOTONIC_RAW, &finish); /* end of measurement */

            /* print the results of the text processing */
            printResults(filesData, numFiles);

            /* calculate the elapsed time */
            printf("\nElapsed time = %.6f s\n", (finish.tv_sec - start.tv_sec) / 1.0 + (finish.tv_nsec - start.tv_nsec) / 1000000000.0);
        }

    }
    else
    {
        MPI_Bcast(&CHUNK_BYTE_LIMIT, 1, MPI_INT, 0, MPI_COMM_WORLD);

        /* allocating memory for the file data structure */
        struct fileData *data = (struct fileData *)malloc(sizeof(struct fileData));
        data->nWords = 0;
        data->nWordsBV = 0;
        data->nWordsEC = 0;
        /* allocating memory for the chunk buffer */
        data->chunk = (unsigned char *)malloc(CHUNK_BYTE_LIMIT * sizeof(unsigned char));

        while (true)
        {
        MPI_Recv(&workStatus, 1, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (workStatus == ALL_FILES_PROCESSED)
            break;

        MPI_Recv(data->chunk, CHUNK_BYTE_LIMIT, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&data->chunkSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&data->previousCh, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        /* perform text processing on the chunk */
        processChunk(data);
        /* Send the processing results to the dispatcher */
        MPI_Send(&data->nWords, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&data->nWordsBV, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&data->nWordsEC, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        /* reset structures */
        data->nWords = 0;
        data->nWordsBV = 0;
        data->nWordsEC = 0;
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
    printf("\t-t <num_threads> : Number of threads to be used (1-8)\n");
    printf("\t-f <file1> <file2> ... <fileN> : List of files to be processed\n");
    printf("\t-c <chunk_size> : Chunk size (4k or 8k)\n");
}