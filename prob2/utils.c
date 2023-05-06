
#include <stdlib.h>
#include <math.h>

#include "sharedRegion.h"
#include "sharedRegion.c"

/**
 * @brief Resets the file structure for the next iteration of the program (new number of threads)
 *
 */


extern struct file *files;

/* Number of files to be processed */
extern int numFiles;

/* Stores the index of the current file being proccessed by the working threads */
int currentFileIndex = 0;

/* Bool to check if all files were processed */
extern bool filesFinished;

/* Size - Number of processes (including root) */
extern int size;


/**
 * @brief Get the text file names by processing the command line and storing them for future retrieval/update by processes
 * 
 * @param filenames 
 * @param numNumbers 
 * @param size 
 */
void storeFilenames(char *filenames[], unsigned int numNumbers[], int size) {
    
    // Allocate memory dynamically using the malloc function to create an array of numFiles elements of struct 'file' type
    files = (struct fileInfo *) malloc(numFiles * sizeof(struct fileInfo));

    // Initialize each element of 'files' of struct fileInfo
    for (int i = 0; i < numFiles; i++) {

        memset((files + i), 0, sizeof(struct fileInfo));
        (files + i)->filename = filenames[i];
        (files + i)->fp = fopen((files + i)->filename, "rb");
        if ( (files + i)->fp == NULL) {
            printf("[ERROR] Can't open file %s\n", (files + i)->filename);
            exit(1);
        }
        (files + i)->fileIndex = i;
        fread(&(files + i)->filename->numNumbers, sizeof(int), 1, file);
        (files + i)->chunkSize = (unsigned int) ceil((files + i)->filename->numNumbers / (size - 1)); /* Not counting with the root process */
        (files + i)->allSequences = (struct Sequence**)malloc((size-1) * sizeof(struct Sequence));

        for (int j = 0; j < sizes-1; j++) {

                        /*Allocate memory for the chunk */
            memset((files + i)->allSequences[j], 0, sizeof(struct Sequence));
            (files + i)->allSequences[j]->sequence = (unsigned int *) malloc(sequenceSize * sizeof(unsigned int));
            (files + i)->allSequences[j]->status = SEQUENCE_UNSORTED;
            /* Get chunk data */
            (files + i)->allSequences[j]->size = sequenceSize;
            /* Last worker can have a different sequence size, since the sequences could not be splitted equally through all workers */
            if (j == size - 1) {
                (files + i)->allSequences[j]->size = (files + currentFileIndex)->numNumbers - (chunkNumbers * (size - 2));
            }

        }

        (files + i)->isFinished = 0;

        int j = 0;
        while (fread(&(files + i)->fullSequence[j], sizeof(unsigned int), 1, (files + i)->fp) == 1) {
            j++;
        }
        fclose((files + i)->fp);

        // Print the numbers in the array
        printf("Numbers in the array:\n");
        for (int k = 0; k < (files + i)->numNumbers; k++) {
            printf("%d ", (files + i)->fullSequence[k]);
        }
        printf("\n");

    }

}

/**
 * @brief Function to return the index position of the Sequence structure that will be sent to the worker thread for future processing
 * 
 * First, it looks for a Sequence in the allSequences variable which is unsorted (first step of the merge sort process), with the status
 * SEQUENCE_UNSORTED. If that's the case, we will fill the sequence variable, which will store the unsorted chunk of integers and return
 * the index of that Sequence structure to send it to the workers.
 *
 * If all sequences are already sorted (the program does not return in the first loop), we should look for two already sorted chunks/sequences
 * that can be merged into one. If we have sequence A and B, the sequence B will be 'appended' to sequence A. Because of that, we can make sequence
 * B obsolete (it wont be used in the future), since sequence A is the merged solution of A and B.
 *
 * If the conditions explained above doesn't happen, there isn't any chunk to get (we already have a merged sequence that corresponds to the file sorted
 * array of integers), returning -1.
 * 
 * @return int 
 */
int getChunk() {

    /* There are files still remanining to be processed */
    if (currentFileIndex < numFiles) {

        /* Search for unsorted sequences that need to be sorted */
        for (int idx = 0; idx < sizes-1; idx++) {

            if ((files + currentFileIndex)->allSequences[idx]->status == SEQUENCE_UNSORTED) {

                for (int i = 0; i < (files + currentFileIndex)->allSequences[idx]->size; i++) {
                    (files + currentFileIndex)->allSequences[idx]->sequence[i] = (files + currentFileIndex)->fullSequence[idx * (files + currentFileIndex)->allSequences[idx] + i];
                }

                (files + currentFileIndex)->allSequences[idx]->status = SEQUENCE_BEING_SORTED;
                return idx;
            }

        }

        
        int sequencesToMerge[2] = {-1};
        int sequenceToMergeIdx = 0;

        /* Search for two sequences that need to be merged (with -2 values) */
        for (int i = 0; i < size - 1; i++) {

            /* Check for two sorted sequences that can be merged together */
            if ((files + currentFileIndex)->allSequences[idx]->mergedSequences[i] == SEQUENCE_SORTED) {
                sequencesToMerge[sequenceToMergeIdx++] = sequenceIdx;
            } 

            /* Already found to sequences to merge! */
            if (sequenceToMergeIdx >= 2) {
                ((files + currentFileIndex)->allSequences[sequencesToMerge[0]])->status = SEQUENCE_BEING_MERGED;   /* Make second sequence obsolete, as it will be merged into the 1st one */
                ((files + currentFileIndex)->allSequences[sequencesToMerge[1]])->status = SEQUENCE_OBSOLETE;   /* Make second sequence obsolete, as it will be merged into the 1st one */
                ((files + currentFileIndex)->allSequences[sequencesToMerge[0]])->size += (sequence + sequencesToMerge[1])->size    /* New sequence will have the size of both sequences size */
                ((files + currentFileIndex)->allSequences[sequencesToMerge[0]])->sequence = (unsigned int *) realloc(((files + currentFileIndex)->allSequences[sequencesToMerge[0]])->size * unsigned int);
                for (int j = 0; j < ((files + currentFileIndex)->allSequences[sequencesToMerge[1]])->size; j++) {
                    ((files + currentFileIndex)->allSequences[sequencesToMerge[0]])->sequence[ ((files + currentFileIndex)->allSequences[sequencesToMerge[0]])->size + j] = ((files + currentFileIndex)->allSequences[sequencesToMerge[1]])->sequence[j];
                }
                return sequencesToMerge[0];
            }

        }


        /* Nothing to process */
        return -1;

    }

}

/**
 * @brief Get the Results object
 *
 */
int validation() {

    for (int i = 0; i < (files + currentFileIndex)->numberOfValues - 1; i++)    {

        if ((files + currentFileIndex)->sortedList[i] > (files + currentFileIndex)->sortedList[i + 1]) {
            printf("Error in position %d between element %d and %d\n", i, (files + currentFileIndex)->sortedList[i], (files + currentFileIndex)->sortedList[i + 1]);
            return -1;
        }    

        if (i == ((files + currentFileIndex)->numberOfValues - 1)){
            printf("Everything is fine\n");
            return 1;
        }

    }
    return 0;

}



void resetChunkData(struct Sequence *sequence) {
    memset(&sequence, 0, sizeof(Sequence));
}



void resetFilesData(struct fileInfo *files) {
    memset(&files, 0, sizeof(fileInfo));
}



void bitonic_merge(int arr[], int low, int cnt, int dir) {
    if (cnt > 1) {
        int k = cnt / 2;
        for (int i = low; i < low + k; i++) {
            if (1 == (arr[i] > arr[i + k])) {
                int temp = arr[i];
                arr[i] = arr[i + k];
                arr[i + k] = temp;
            }
        }
        bitonic_merge(arr, low, k, dir);
        bitonic_merge(arr, low + k, k, dir);
    }
}

void bitonic_sort_recursive(int arr[], int low, int cnt, int dir) {
    if (cnt > 1) {
        int k = cnt / 2;
        bitonic_sort_recursive(arr, low, k, 1);
        bitonic_sort_recursive(arr, low + k, k, 0);
        bitonic_merge(arr, low, cnt, dir);
    }
}

void bitonic_sort(int arr[], int n) {
    int cnt = 2;
    while (cnt <= n) {
        for (int i = 0; i < n; i += cnt) {
            bitonic_sort_recursive(arr, i, cnt, 1);
        }
        cnt *= 2;
    }
}

void merge_sorted_arrays(int *arr1, int n1, int *arr2, int n2, int *result) {
    int i = 0, j = 0, k = 0;

    while (i < n1 && j < n2) {
        if (arr1[i] <= arr2[j]) {
            result[k++] = arr1[i++];
        } else {
            result[k++] = arr2[j++];
        }
    }

    while (i < n1) {
        result[k++] = arr1[i++];
    }

    while (j < n2) {
        result[k++] = arr2[j++];
    }

    //int *result = (int *)malloc((n1 + n2) * sizeof(int));

    //merge_sorted_arrays(arr1, n1, arr2, n2, result);
}

