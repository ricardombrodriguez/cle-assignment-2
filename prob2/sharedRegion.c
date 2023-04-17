/**
 * @file sharedRegion.c
 *  @author Pedro Sobral & Ricardo Rodriguez, March 2023
 * @brief
 * @date 2023-03-14
 *
 * @copyright Copyright (c) 2023
 *
 */


#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "constants.h"
#include "utils.h"
#include "sharedRegion.h"

/* Number of files to be processed */
extern int numFiles;

/* Number of threads */
extern int numThreads;

/* Stores the index of the current file being proccessed by the working threads */
int currentFileIndex = 0;

/* Stores working thread status */
extern int *threadStatus;

/* Mutex lock attribute */
static pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

/* File sructure initialization */
struct file *files;

/* Bool to check if all files were processed */
extern bool filesFinished;

/* Stores the current position of the file chunk */
int bufferIndex = 0;



/**
 * @brief Get the text file names by processing the command line and storing them in the shared region for future retrieval by worker threads
 *
 * @param filenames
 */
void storeFilenames(char *filenames[], unsigned int numNumbers[]) {
    
    // Allocate memory dynamically using the malloc function to create an array of numFiles elements of struct 'file' type
    files = (struct file *)malloc(numFiles * sizeof(struct file));

    // Initialize each element of 'files' of struct file
    for (int i = 0; i < numFiles; i++) {

        memset((files + i), 0, sizeof(struct file));
        (files + i)->filename = filenames[i];
        (files + i)->numNumbers = numNumbers[i];
        (files + i)->chunkSize = (unsigned int)ceil(numNumbers[i] / numThreads); /* Not counting with the distributer thread */
        (files + i)->sortedList = (int *)malloc(numNumbers[i] * sizeof(int));
        (files + i)->fp = fopen((files + i)->filename, "rb");
        if ((files + i)->fp == NULL) {
            printf("[ERROR] Can't open file %s\n", (files + i)->filename);
            exit(1);
        }

        int j = 0;
        while (fread(&(files + i)->sortedList[j], sizeof(int), 1, (files + i)->fp) == 1) {
            j++;
        }

        fclose((files + i)->fp);

        // Print the numbers in the array
        printf("Numbers in the array:\n");
        for (int k = 0; k < (files + i)->numNumbers; k++) {
            printf("%d ", (files + i)->sortedList[k]);
        }
        printf("\n");

    }

}



/**
 * @brief
 *
 * @param chunkData
 * @param threadID
 */
void saveThreadResults(struct chunk *chunkData, unsigned int threadID) {

    int error;

    /* Mutex lock */
    if ((error = pthread_mutex_lock(&count_mutex)) != 0) {
        printf("[ERROR] Can't enter the mutex critical zone.");
        pthread_exit(0);
    }

    for (int i = 0; i < chunkData->chunkSize; i++) {

        (files + currentFileIndex)->sortedList[bufferIndex+i] = chunkData->chunkList[i];

    }

    //printf("[THREAD %d] File %s: %d words, %d words with A vowel, %d words with E vowels", threadID, (files + chunkData->fileIndex)->filename, chunkData->numWords, chunkData->nWordsWithVowel[0], chunkData->nWordsWithVowel[1]);

    /* Mutex unlock */
    if ((pthread_mutex_unlock(&count_mutex)) != 0) {
        printf("[ERROR] Can't leave the mutex critical zone.");
        pthread_exit(0);
    }
}

/**
 * @brief Get the Chunk object
 *
 * @param chunkData
 * @param threadID
 */
struct chunk getChunk(unsigned int threadID) {

    int error;

    /* Mutex lock */
    if ((error = pthread_mutex_lock(&count_mutex)) != 0) {
        printf("[ERROR] Can't enter the mutex critical zone.");
        pthread_exit(0);
    }

    /* Allocate memory to support a chunk structure */
    struct chunk *chunkData = (struct chunk *)malloc(sizeof(struct chunk));
    chunkData->chunkList = (int *) malloc(files->chunkSize * sizeof(int));
    chunkData->sortedListOffset = bufferIndex;

    for (int i = 0; i < (files + currentFileIndex)->chunkSize; i++) {

        if ( (bufferIndex + i) >= (files + currentFileIndex)->numberOfValues ) {
            fclose((files + currentFileIndex)->fp);
            chunkData->isFinished = true;
            chunkData->chunkSize = i + 1;       /* Chunk with smaller size than usual */
            bufferIndex = 0;
            break;
        }

        chunkData->chunkList[i] = (files + currentFileIndex)->sortedList[bufferIndex+i];
        bufferIndex++;

    }

    /* Mutex unlock */
    if ((pthread_mutex_unlock(&count_mutex)) != 0) {
        printf("[ERROR] Can't leave the mutex critical zone.");
        pthread_exit(0);
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