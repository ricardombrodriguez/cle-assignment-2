
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
struct fileInfo *files;

/* Stores the index of the current file being proccessed by the working threads */
int currentFileIndex = 0;

int isFinished;

int size;


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
    int rank;                       /* Rank - Process ID | Size - Number of processes (including root) */
    unsigned int sequenceSize, sequenceSizeRemainder;

    /* Initialize the MPI communicator and get the rank of processes and the count of processes */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* ERROR. Number of processes should be limited. */
    if (size < 2 || size > 8) {
        fprintf(stderr, "Invalid number of processes (must be >= 2 and <= 8)");
        return EXIT_FAILURE;
    }
  
    /* ERROR. No arguments for filenames were introduced to the program. */
    if (argc < 2) {
        printf("[ERROR] Invalid number of files");
        return 1;
    }

    const char *optstr = "f:h";
    int option;
    char *filenames[MAX_NUM_FILES];

    if (rank == 0) {

        /* Structure used to keep track of the execution time */
        struct timespec start, finish;
        
        /* Clock start */
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);

        while ((option = getopt(argc, argv, optstr)) != -1) {
            switch (option) {
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
        storeFilenames(filenames, size);

        /* Keep track of the current worker's rank ID */
        int nWorkers = 0;

        /* Wait for chunk requests while there are still files to be processed */
        while (!isFinished)
        {

            sequenceSize = (files + currentFileIndex)->numNumbers / (size - 1);
            sequenceSizeRemainder = (files + currentFileIndex)->numNumbers % (size - 1);
            if (sequenceSizeRemainder != 0) {
                sequenceSize++;
            }

            /* Broadcast message to working processes, so that they can start asking for integer chunks */
            MPI_Bcast(&sequenceSize, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);

            int sequenceIdx, fileIdx;

            /* For all the worker processes */
            for (nWorkers = 1; nWorkers < size; nWorkers++)
            {

                sequenceIdx = getChunk();

                printf("[ROOT] -> [RANK %d] IS FINISHED ----> %d\n", nWorkers, isFinished);
       
                MPI_Send(&isFinished, 1, MPI_INT, nWorkers, MPI_TAG_PROGRAM_STATE, MPI_COMM_WORLD);

                fileIdx = currentFileIndex;
                if ((files + currentFileIndex)->allSequences[sequenceIdx]->status == SEQUENCE_FINAL) {
                    currentFileIndex++;
                    if (currentFileIndex >= numFiles) {
                        isFinished = 1;
                    }
                }

                MPI_Send(&currentFileIndex, 1, MPI_INT, nWorkers, MPI_TAG_CHUNK_REQUEST, MPI_COMM_WORLD);
                MPI_Send(&(files + fileIdx)->allSequences[sequenceIdx]->size, 1, MPI_UNSIGNED, nWorkers, MPI_TAG_CHUNK_REQUEST, MPI_COMM_WORLD);
                MPI_Send((files + fileIdx)->allSequences[sequenceIdx]->sequence, (files + fileIdx)->allSequences[sequenceIdx]->size, MPI_UNSIGNED, nWorkers, MPI_TAG_CHUNK_REQUEST, MPI_COMM_WORLD);     
                MPI_Send(&(files + fileIdx)->allSequences[sequenceIdx]->status, 1, MPI_INT, nWorkers, MPI_TAG_CHUNK_REQUEST, MPI_COMM_WORLD);             
                MPI_Send(&sequenceIdx, 1, MPI_INT, nWorkers, MPI_TAG_CHUNK_REQUEST, MPI_COMM_WORLD);  

                printf("[ROOT] enviei tudo para o %d com um sequenceSize = %d \n", nWorkers, (files + fileIdx)->allSequences[sequenceIdx]->size);
        
            }

            /* For all the worker processes */
            for (int i = 1; i < nWorkers; i++)
            {
                /* Allocate memory to support a Sequence structure */
                struct Sequence *seqData = (struct Sequence *) malloc(sizeof(struct Sequence));

                /*Allocate memory for the chunk */
                seqData->sequence = (unsigned int *)malloc(sequenceSize * sizeof(unsigned int));
                seqData->status = SEQUENCE_UNSORTED;
                seqData->size = 0;
                int fileIdx;

                /* Receive the processing results from each worker process */
                MPI_Recv(&fileIdx, 1, MPI_INT, i, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&seqData->size, 1, MPI_UNSIGNED, i, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(seqData->sequence, seqData->size, MPI_UNSIGNED, i, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&seqData->status, 1, MPI_INT, i, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&sequenceIdx, 1, MPI_INT, i, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                printf("[ROOT] Recebi a sequencia com status %d do idx %d (SIZE DO CHUNK = %u) | FILE INDEX = %d\n", seqData->status, sequenceIdx, seqData->size, fileIdx);


                (files + fileIdx)->allSequences[sequenceIdx] = seqData;

                if (isFinished)
                    break;


            }

        }

        printf("[ROOT] ACABOU | A ENVIAR TUDO!\n");

		/* Inform workers that all files are process and they can exit */
		for (int i = 1; i < size; i++) {
		    MPI_Send(&isFinished, 1, MPI_INT, i, MPI_TAG_PROGRAM_STATE, MPI_COMM_WORLD);
        }

        /* Clock end */
        clock_gettime(CLOCK_MONOTONIC_RAW, &finish);

        /* Print the results of the text processing of all files */
        validation();

        /* Calculate execution time */
        printf("\eExecution time = %.6f s\n", (finish.tv_sec - start.tv_sec) / 1.0 + (finish.tv_nsec - start.tv_nsec) / 1000000000.0);

    } else {

        /* Receive brodcast message from the dispatcher */
        MPI_Bcast(&sequenceSize, 1, MPI_INT, 0, MPI_COMM_WORLD);

        /* Allocate memory to support a Sequence structure */
        struct Sequence *seqData = (struct Sequence *) malloc(sizeof(struct Sequence));

        /*Allocate memory for the chunk */
        seqData->sequence = (unsigned int *)malloc(sequenceSize * sizeof(unsigned int));
        seqData->status = SEQUENCE_UNSORTED;
        seqData->size = 0;
        int sequenceIdx, fileIdx;

        while (true)
        {
    
            /* Receive the control variable from the dispatcher to know if there's work still to be done (or not) */
            MPI_Recv(&isFinished, 1, MPI_INT, 0, MPI_TAG_PROGRAM_STATE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            /* End worker if all files have been processed */
            if (isFinished) 
                break;

            MPI_Recv(&fileIdx, 1, MPI_INT, 0, MPI_TAG_CHUNK_REQUEST, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&seqData->size, 1, MPI_UNSIGNED,  0, MPI_TAG_CHUNK_REQUEST, MPI_COMM_WORLD, MPI_STATUS_IGNORE);                   
            MPI_Recv(seqData->sequence, seqData->size, MPI_UNSIGNED,  0, MPI_TAG_CHUNK_REQUEST, MPI_COMM_WORLD, MPI_STATUS_IGNORE);     
            MPI_Recv(&seqData->status, 1, MPI_INT,  0, MPI_TAG_CHUNK_REQUEST, MPI_COMM_WORLD, MPI_STATUS_IGNORE);     
            MPI_Recv(&sequenceIdx, 1, MPI_INT,  0, MPI_TAG_CHUNK_REQUEST, MPI_COMM_WORLD, MPI_STATUS_IGNORE);                                     

            processChunk(seqData);

            MPI_Send(&fileIdx, 1, MPI_INT, 0, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD);
            MPI_Send(&seqData->size, 1, MPI_UNSIGNED, 0, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD);                    
            MPI_Send(seqData->sequence, seqData->size, MPI_UNSIGNED, 0, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD);    
            MPI_Send(&seqData->status, 1, MPI_INT, 0, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD);  
            MPI_Send(&sequenceIdx, 1, MPI_INT, 0, MPI_TAG_SEND_RESULTS, MPI_COMM_WORLD);          

            printf("[RANK %d] sent sorted chunk to root with size %u. status = %d\n", rank, seqData->size, seqData->status);         

            /* Reset chunk data */
            resetChunkData(seqData);

            /* End worker if all files have been processed */
            if (isFinished)
                break;
            
        }

    }

    printf("[RANK %d] FINALIZEEEEEEEEE\n\n", rank);

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

