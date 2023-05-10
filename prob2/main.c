/**
 * @file main.c
 * @authors Pedro Sobral, Ricardo Rodriguez
 * @brief Parallel Bitonic Sort using MPI
 *
 * This program reads integers from binary files and sorts them using the bitonic sort algorithm
 * in parallel with the Message Passing Interface (MPI) library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>

#include "utils.h"

#define DISTRIBUTOR_RANK 0

/**
 * @brief Main function of the program.
 * 
 */
int main(int argc, char *argv[])
{
    
    MPI_Init(&argc, &argv);

    /* MPI related variables */
    int size, rank; 
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    double start_time = 0.0, end_time = 0.0;
    double total_start_time = 0.0, total_end_time = 0.0;

    total_start_time = MPI_Wtime();

    /* Check for usage errors (should have at least one file) */
    if (argc < 3 || strcmp(argv[1], "-f") != 0)
    {
        if (rank == DISTRIBUTOR_RANK)
        {
            fprintf(stderr, "[ERROR] Usage: %s -f <file1> [<file2> ...]\n", argv[0]);
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    /* Sort each file at a time */
    for (int i = 2; i < argc; i++)
    {

        FILE *file = NULL;
        int numValues = 0;

        /* Distributor is responsible for opening the file and read the number of values/integers of it */
        if (rank == DISTRIBUTOR_RANK)
        {
            file = fopen(argv[i], "rb");
            if (!file)
            {
                fprintf(stderr, "[ERROR] Error opening file: %s\n", argv[i]);
                continue;
            }
            fread(&numValues, sizeof(int), 1, file);

            printf("Processing file: %s\n", argv[i]);
            start_time = MPI_Wtime();
        }



        /* Broadcast the number of values of the file to other processes */
        MPI_Bcast(&numValues, 1, MPI_INT, 0, MPI_COMM_WORLD);

        /* Allocate memory to hold the integer array */
        int *array = (int *)malloc(numValues * sizeof(int));

        /* Distributor is responsible for reading all the file's integers and store it in the array */
        if (rank == DISTRIBUTOR_RANK)
        {
            fread(array, sizeof(int), numValues, file);
            fclose(file);
        }

        /* Broadcast the integer array to other processes */
        MPI_B∏
        
        cast(array, numValues, MPI_INT, 0, MPI_COMM_WORLD);

        int *localArray = (int *)malloc(numValues * sizeof(int)); /* Allocate memory for the local array */
        int chunkSize = numValues / size; /* The chunk size is equally reparted according to the number of processes (size) */

        /* MPI_Scatter sends each process a part of the input array (with chunkSize numbers) and stores it to the localArray buffer */
        MPI_Scatter(array, chunkSize, MPI_INT, localArray, chunkSize, MPI_INT, 0, MPI_COMM_WORLD);

        /* Perform bitonic merge sort on the localArray */
        bitonicMergeSort(localArray, 0, chunkSize, 1);

        /* Iterate over the powers of 2 (1,2,4,...) */
        for (int i = 1; i < size; i <<= 1)
        {
            /* Find partner using the XOR operator. This partner rank is the rank of the process that the current process will communicate with during the current iteration. */
            int partner = rank ^ i;

            /* Send the current process's localArray data to the partner process and receives the partner's localArray data in return (mutual exchange) */
            MPI_Sendrecv_replace(localArray, chunkSize, MPI_INT, partner, 0, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            /** 
             * Compute the direction in which the bitonicMerge function will merge the localArray data. 
             * If (rank & i) != 0 -> direction = 0 -> merge should be in ascending order. 
             * If (rank & i) == 0 -> direction = 1 -> merge should be in descending order. 
            */
            int direction = (rank & i) ? 0 : 1;

            /* Perform the bitonic merge according to the previous computed direction on the localArray*/
            bitonicMerge(localArray, 0, chunkSize, direction);
        }

        /* Gather all the local arrays from all the processes and merge them into the final array */
        MPI_Gather(localArray, chunkSize, MPI_INT, array, chunkSize, MPI_INT, 0, MPI_COMM_WORLD);

        /* The distributor will use bitonic merge sort to sort all the gathered localArrays of each worker process */
        if (rank == DISTRIBUTOR_RANK)
        {
            bitonicMergeSort(array, 0, numValues, 1);
        }

        /* The distributor validates the sorted array and prints the results and execution times */
        if (rank == DISTRIBUTOR_RANK)
        {
            /* Check if the sorted array is valid */
            if (validation(array, numValues))
            {
                printf("Validation: Array is correctly sorted.\n");
            } 
            else 
            {
                printf("Validation: Array is NOT correctly sorted.\n");
            }

            /* Calculate and print the execution time of the current file */
            end_time = MPI_Wtime();
            printf("[File: %s] | Execution time: %f seconds\n\n", argv[i], end_time - start_time);
        }

    /* Free memory */
    free(localArray);
    free(array);
    
    }

    /* Calculate the execution time of ALL the files*/
    if (rank == DISTRIBUTOR_RANK)
    {
        total_end_time = MPI_Wtime();
        printf("Total execution time: %f seconds\n", total_end_time - total_start_time);
    }

    /* Finalize the process */
    MPI_Finalize();
    return EXIT_SUCCESS;
}