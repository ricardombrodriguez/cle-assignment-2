/**
 *  @file utils.h (interface file)
 *
 *  @brief 
 *
 *  @author Pedro Sobral & Ricardo Rodriguez, March 2023
 */
#ifndef UTILS_H
# define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Information about the file (name and pointer) and reference to the file sorted sequence (final state)
 * 
 */
struct fileInfo {
    char *filename;
    FILE *fp;
    unsigned int fileIndex;
    int numNumbers;
    unsigned int chunkSize;
    unsigned int *fullSequence;
    struct Sequence **allSequences; 
    int isFinished;
};

/**
 * @brief Structure that stores the integer sequence, the size (number of integers of the sequence) and a boolean
 * variable *isSorted* to know if sequence is sorted (or not)
 * 
 */
struct Sequence {
    unsigned int *sequence;
    unsigned int size;
    int status;
};


/**
 * @brief Get the text file names by processing the command line and storing them for future retrieval/update by processes
 * 
 * @param filenames 
 * @param size 
 */
extern void storeFilenames(char *filenames[], int size);

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
extern int getChunk();


extern void processChunk(int sequenceIdx);

extern int validation();

extern void resetChunkData(struct Sequence *sequence);

extern void resetFilesData(struct fileInfo *files);


/** \brief get the determinant of given matrix */
extern double getDeterminant(int order, double *matrix); 

extern void bitonic_merge(unsigned int arr[], int low, int cnt, int dir);

extern void bitonic_sort_recursive(unsigned int arr[], int low, int cnt, int dir);

extern void bitonic_sort(unsigned int arr[], int n);

extern void merge_sorted_arrays(unsigned int *arr1, int n1, unsigned int *arr2, int n2, unsigned int *result);


#endif /* UTILS_H */