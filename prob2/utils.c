
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "utils.h"
#include "constants.h"

/**
 * @brief Resets the file structure for the next iteration of the program (new number of threads)
 *
 */


extern struct fileInfo *files;

/* Number of files to be processed */
extern unsigned int numFiles;

/* Stores the index of the current file being proccessed by the working threads */
extern int currentFileIndex;

/* Bool to check if all files were processed */
extern int isFinished;

/* Size - Number of processes (including root) */
extern int size;

int chunkIdx = 0;


/**
 * @brief Get the text file names by processing the command line and storing them for future retrieval/update by processes
 * 
 * @param filenames 
 * @param size 
 */
void storeFilenames(char *filenames[], int size) {
    
    // Allocate memory dynamically using the malloc function to create an array of numFiles elements of struct 'file' type
    files = (struct fileInfo *) malloc(numFiles * sizeof(struct fileInfo));

    // Initialize each element of 'files' of struct fileInfo
    for (int i = 0; i < numFiles; i++) {

        memset((files + i), 0, sizeof(struct fileInfo));
        (files + i)->filename = filenames[i];
        (files + i)->fp = fopen((files + i)->filename, "rb");
        if ( (files + i)->fp == NULL) {
            printf("[ERROR] Can't open file %s\n", (files + i)->filename);
            exit(EXIT_FAILURE);
        }

        (files + i)->fileIndex = i;
<<<<<<< HEAD
        if (!fread(&(files + i)->numNumbers, sizeof(unsigned int), 1, (files + i)->fp )) {
=======
        if (fread(&(files + i)->numNumbers, sizeof(int), 1,(files + i)->fp ) != 1) {

>>>>>>> 8a147fdfbdcc1b562269c13bf16f83eb5c8185fd
            printf("[ERROR] Can't read the first line of the file\n");
            exit(EXIT_FAILURE);
        }
        (files + i)->chunkSize = (unsigned int) ceil((files + i)->numNumbers / (size - 1)); /* Not counting with the root process */
<<<<<<< HEAD
        (files + i)->allSequences = (struct Sequence**)malloc((size-1) * sizeof(struct Sequence));
        (files + i)->fullSequence = (unsigned int *) malloc((files + i)->numNumbers * sizeof(unsigned int));


        for (int j = 0; j < size-1; j++) {

            struct Sequence *sequence = (struct Sequence *) malloc(sizeof(struct Sequence));
            sequence->sequence = (unsigned int *) malloc((files + i)->chunkSize * sizeof(unsigned int));
            sequence->status = SEQUENCE_UNSORTED;
            sequence->size = (files + i)->chunkSize;
=======
        (files + i)->allSequences = (struct Sequence**)malloc((size-1) * sizeof(struct Sequence*));


        for (int j = 0; j < size-1; j++) {
            /*Allocate memory for the chunk */
            (files + i)->allSequences[j] = (struct Sequence*)malloc(sizeof(struct Sequence));
            memset((files + i)->allSequences[j], 0, sizeof(struct Sequence));
            (files + i)->allSequences[j]->sequence = (unsigned int *) malloc((files + i)->chunkSize * sizeof(unsigned int));
            (files + i)->allSequences[j]->status = SEQUENCE_UNSORTED;
            /* Get chunk data */
            (files + i)->allSequences[j]->size = (files + i)->chunkSize;
            /* Last worker can have a different sequence size, since the sequences could not be splitted equally through all workers */
>>>>>>> 8a147fdfbdcc1b562269c13bf16f83eb5c8185fd
            if (j == size - 1) {
                sequence->size = (files + i)->numNumbers - ((files + i)->chunkSize * (size - 2));
            }
<<<<<<< HEAD
            (files + i)->allSequences[j] = sequence;
            //memcpy((files + i)->allSequences[j], sequence, sizeof(struct Sequence));


        }
=======
        }
        
>>>>>>> 8a147fdfbdcc1b562269c13bf16f83eb5c8185fd
        (files + i)->isFinished = 0;

        /* Allocate memory for the full sequence */
        (files + i)->fullSequence = (unsigned int *) malloc((files + i)->numNumbers * sizeof(unsigned int));


        int j = 0;
        while (fread(&(files + i)->fullSequence[j], sizeof(unsigned int), 1, (files + i)->fp) == 1) {
            j++;
        }
        fclose((files + i)->fp);

        // Print the numbers in the array
        //printf("Numbers in the array:\n");
        //for (int k = 0; k < (files + i)->numNumbers; k++) {
        //    printf("%d ", (files + i)->fullSequence[k]);
        //}
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
        for (int idx = 0; idx < size-1; idx++) {

            if ((files + currentFileIndex)->allSequences[idx]->status == SEQUENCE_UNSORTED) {

                for (int i = 0; i < (files + currentFileIndex)->allSequences[idx]->size; i++) {
                    (files + currentFileIndex)->allSequences[idx]->sequence[i] = (files + currentFileIndex)->fullSequence[(idx * (files + currentFileIndex)->chunkSize) + i];
                }

                (files + currentFileIndex)->allSequences[idx]->status = SEQUENCE_BEING_SORTED;
                return idx;
            } else {
                printf("STATUS = %d \n", (files + currentFileIndex)->allSequences[idx]->status);
            }

        }

        
        int sequencesToMerge[2] = {-1};
        int sequenceToMergeIdx = 0;

        /* Search for two sequences that need to be merged (with -2 values) */
        for (int i = chunkIdx; i < size - 1; i++) {

            /* Check for two sorted sequences that can be merged together */
            if ((files + currentFileIndex)->allSequences[i]->status == SEQUENCE_SORTED) {
                sequencesToMerge[sequenceToMergeIdx++] = i;
            } 

            chunkIdx++;
            if (chunkIdx >= size - 1) 
                chunkIdx = 0;

            /* Already found to sequences to merge! */
            if (sequenceToMergeIdx >= 2) {

                mergeSequences(((files + currentFileIndex)->allSequences[sequencesToMerge[0]]),((files + currentFileIndex)->allSequences[sequencesToMerge[1]]));

                int isFinalSequence = 1;

                /* If it's the final sequence (file sorted), all of the other sequences should have the status SEQUENCE_OBSOLETE */
                for (int j = 0; j < size-1; j++) {
                    if (j == sequencesToMerge[0]) continue;
                    if (((files + currentFileIndex)->allSequences[j])->status != SEQUENCE_OBSOLETE) {
                        isFinalSequence = 0;
                        break;
                    }
                }
                if (isFinalSequence) {
                    ((files + currentFileIndex)->allSequences[sequencesToMerge[0]])->status = SEQUENCE_FINAL;
                }

                return sequencesToMerge[0];
            }

        }

    }

    /* Nothing to process */
    return -1;

}


void processChunk(struct Sequence *sequence) {

    mergeSortWrapper(&sequence->sequence, sequence->size);

    if (sequence->status == SEQUENCE_BEING_SORTED || sequence->status == SEQUENCE_BEING_MERGED) {
        printf("-- Sequence is sorted!\n");
        sequence->status = SEQUENCE_SORTED;
    }

}



/**
 * @brief Get the Results object
 *
 */
int validation() {

    for (int i = 0; i < (files + currentFileIndex)->numNumbers - 1; i++)    {

        if ((files + currentFileIndex)->fullSequence[i] > (files + currentFileIndex)->fullSequence[i + 1]) {
            printf("Error in position %d between element %d and %d\n", i, (files + currentFileIndex)->fullSequence[i], (files + currentFileIndex)->fullSequence[i + 1]);
            return -1;
        }    

        if (i == ((files + currentFileIndex)->numNumbers - 1)){
            printf("Everything is fine\n");
            return 1;
        }

    }
    return 0;

}



void resetChunkData(struct Sequence *sequence) {
    memset(sequence, 0, sizeof(struct Sequence));
}



void resetFilesData(struct fileInfo *files) {
    memset(files, 0, sizeof(struct fileInfo));
}



void bitonic_merge(unsigned int arr[], unsigned int low, unsigned int cnt, unsigned int dir) {
    if (cnt > 1) {
        unsigned int k = cnt / 2;
        for (unsigned int i = low; i < low + k; i++) {
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

void bitonic_sort_recursive(unsigned int arr[], unsigned int low, unsigned int cnt, unsigned int dir) {
    if (cnt > 1) {
        unsigned int k = cnt / 2;
        bitonic_sort_recursive(arr, low, k, 1);
        bitonic_sort_recursive(arr, low + k, k, 0);
        bitonic_merge(arr, low, cnt, dir);
    }
}

void bitonic_sort(unsigned int arr[], unsigned int n) {
    unsigned int cnt = 2;
    while (cnt <= n) {
        for (unsigned int i = 0; i < n; i += cnt) {
            bitonic_sort_recursive(arr, i, cnt, 1);
        }
        cnt *= 2;
    }
}





void merge(unsigned int arr[], unsigned int left[], unsigned int leftSize, unsigned int right[], unsigned int rightSize) {
    unsigned int i = 0, j = 0, k = 0;
    
    while (i < leftSize && j < rightSize) {
        if (left[i] <= right[j]) {
            arr[k] = left[i];
            i++;
        }
        else {
            arr[k] = right[j];
            j++;
        }
        k++;
    }
    
    while (i < leftSize) {
        arr[k] = left[i];
        i++;
        k++;
    }
    
    while (j < rightSize) {
        arr[k] = right[j];
        j++;
        k++;
    }
}

void mergeSort(unsigned int arr[], unsigned int size) {
    if (size < 2) {
        return;
    }
    
    unsigned int mid = size / 2;
    unsigned int left[mid];
    unsigned int right[size - mid];
    
    for (unsigned int i = 0; i < mid; i++) {
        left[i] = arr[i];
    }
    
    for (unsigned int i = mid; i < size; i++) {
        right[i - mid] = arr[i];
    }
    
    mergeSort(left, mid);
    mergeSort(right, size - mid);
    merge(arr, left, mid, right, size - mid);
}

void mergeSortWrapper(unsigned int *arr[], unsigned int size) {
    if (arr == NULL || *arr == NULL || size == 0) {
        return;
    }
    
    unsigned int *temp = *arr;
    mergeSort(temp, size);
}

void mergeSequences(struct Sequence *seq1, struct Sequence *seq2) {
    int size1 = seq1->size;
    int size2 = seq2->size;
    unsigned int *result = (unsigned int *)malloc((size1 + size2) * sizeof(unsigned int));
    int i = 0, j = 0, k = 0;

    // while (i < size1 && j < size2) {
    //     if (seq1->sequence[i] < seq2->sequence[j]) {
    //         result[k++] = seq1->sequence[i++];
    //     } else {
    //         result[k++] = seq2->sequence[j++];
    //     }
    // }

    while (i < size1) {
        result[k++] = seq1->sequence[i++];
    }

    while (j < size2) {
        result[k++] = seq2->sequence[j++];
    }

    // Update the first sequence with the merged result and update its size
    free(seq1->sequence);
    seq1->sequence = result;
    seq1->size = size1 + size2;
    seq1->status = SEQUENCE_SORTED;
    seq2->status = SEQUENCE_OBSOLETE;
}