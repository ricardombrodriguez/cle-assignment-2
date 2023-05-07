#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>

void bitonicMerge(int *arr, int low, int count, int direction);
void bitonicMergeSort(int *arr, int low, int count, int direction);

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int num_procs, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc < 3 || strcmp(argv[1], "-f") != 0)
    {
        if (rank == 0)
        {
            fprintf(stderr, "Usage: %s -f <file1> [<file2> ...]\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    for (int i = 2; i < argc; i++)
    {
        if (rank == 0)
        {
            printf("Processing file: %s\n", argv[i]);
        }

        FILE *file = NULL;
        int n = 0;

        if (rank == 0)
        {
            file = fopen(argv[i], "rb");
            if (!file)
            {
                fprintf(stderr, "Error opening file: %s\n", argv[i]);
                continue;
            }
            fread(&n, sizeof(int), 1, file);
        }

        MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

        int *arr = (int *)malloc(n * sizeof(int));
        if (rank == 0)
        {
            fread(arr, sizeof(int), n, file);
            fclose(file);
        }

        MPI_Bcast(arr, n, MPI_INT, 0, MPI_COMM_WORLD);

        int *local_arr = (int *)malloc(n * sizeof(int));
        int chunk_size = n / num_procs;

        MPI_Scatter(arr, chunk_size, MPI_INT, local_arr, chunk_size, MPI_INT, 0, MPI_COMM_WORLD);

        bitonicMergeSort(local_arr, 0, chunk_size, 1);

        for (int i = 1; i < num_procs; i <<= 1)
        {
            int partner = rank ^ i;
            MPI_Sendrecv_replace(local_arr, chunk_size, MPI_INT, partner, 0, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            int direction = (rank & i) ? 0 : 1;
            bitonicMerge(local_arr, 0, chunk_size, direction);
        }

        MPI_Gather(local_arr, chunk_size, MPI_INT, arr, chunk_size, MPI_INT, 0, MPI_COMM_WORLD);

        if (rank == 0)
        {
            bitonicMergeSort(arr, 0, n, 1);
        }

        if (rank == 0)
        {
            printf("Sorted array:\n");
            for (int j = 0; j < n; j++)
            {
                printf("%d\n", arr[j]);
            }
        }

        free(local_arr);
        free(arr);
    }

    MPI_Finalize();
    return 0;
}


void bitonicMerge(int *arr, int low, int count, int direction)
{
if (count > 1)
{
int k = count / 2;
for (int i = low; i < low + k; i++)
{
if (direction == (arr[i] > arr[i + k]))
{
int temp = arr[i];
arr[i] = arr[i + k];
arr[i + k] = temp;
}
}
bitonicMerge(arr, low, k, direction);
bitonicMerge(arr, low + k, k, direction);
}
}

void bitonicMergeSort(int *arr, int low, int count, int direction)
{
if (count > 1)
{
int k = count / 2;
bitonicMergeSort(arr, low, k, 1);
bitonicMergeSort(arr, low + k, k, 0);
bitonicMerge(arr, low, count, direction);
}
}
