/**
 * @file sharedRegion.c
 * @author Pedro Sobral & Ricardo Rodriguez, March 2023

 * @date 2023-03-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "constants.h"
#include "sharedRegion.h"
#include "utils.h"

/* Number of files to be processed */
extern int numFiles;

extern int CHUNK_BYTE_LIMIT;

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

extern bool filesFinished;

int currentQueueIndex;

int finishedWorkers = 0;

bool requestedJob = true;

/**
 * @brief Get the text file names by processing the command line and storing them in the shared region for future retrieval by worker threads
 * 
 * @param filenames 
 */
void storeFilenames(char *filenames[]) {

    // Allocate memory dynamically using the malloc function to create an array of numFiles elements of struct 'file' type
    files = (struct file *) malloc(numFiles * sizeof(struct file));

    // Initialize each element of 'files' of struct file 
    for (int i = 0; i < numFiles; i++) {
        memset((files + i), 0, sizeof(struct file));
        (files + i)->fp = NULL;
        (files + i)->filename = filenames[i];
    }

}

/**
 * @brief Resets the file structure for the next iteration of the program (new number of threads)
 * 
 */
void resetFilesData() {

    files = (struct file *){ 0 };

}


/**
 * @brief 
 * 
 * @param chunkData 
 * @param threadID 
 */
void saveThreadResults(struct fileChunk *chunkData, unsigned int threadID) {

    int error;

    /* Mutex lock */
    if ((error = pthread_mutex_lock(&count_mutex)) != 0) {
        printf("[ERROR] Can't enter the mutex critical zone.");
        pthread_exit(0);
    }

    (files + chunkData->fileIndex)->numWords += chunkData->numWords;
    for (int i = 0; i < 6; i++) {
        (files + chunkData->fileIndex)->nWordsWithVowel[i] += chunkData->nWordsWithVowel[i];
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
void getChunk(struct fileChunk *chunkData, unsigned int threadID) {

    int error;

    /* Mutex lock */
    if ((error = pthread_mutex_lock(&count_mutex)) != 0) {
        printf("[ERROR] Can't enter the mutex critical zone.");
        pthread_exit(0);
    }

    /* There are files still remanining to be processed */
    if (currentFileIndex < numFiles) {

        /* Create file pointer if it doesn't exist (singleton) */
        if ((files + currentFileIndex)->fp == NULL) {

            (files + currentFileIndex)->fp = fopen((files + currentFileIndex)->filename, "rb");
            if ( (files + currentFileIndex)->fp == NULL) {
                printf("[ERROR] Can't open file %s\n", (files + currentFileIndex)->filename);
                exit(1);
            }
        } 

        int byteIndex = 0;
        int dividedWordOffset = 0;
        char *charHexadecimal = (char *) malloc(12 * sizeof(char));    /* Byte limit of hexadecimal string */
        int remainingBytes = 0;

        chunkData->fileIndex = currentFileIndex;
        chunkData->isFinished = false; 

        while (byteIndex < CHUNK_BYTE_LIMIT) {

            int byte = fgetc((files + currentFileIndex)->fp);

            /* EOF byte */
            if (byte == EOF) {

                currentFileIndex++;
                dividedWordOffset = 0;
                chunkData->chunkSize = byteIndex;
                chunkData->isFinished = true;

                if (currentFileIndex == numFiles) {
                    filesFinished = true;
                } 

                break;
            }

            char byteHexadecimal[9];
            sprintf(byteHexadecimal, "%02x", byte);

            /* Initial byte. Calculate how many are left according to UTF-8 standards */
            if (remainingBytes == 0) {
                remainingBytes = getRemainingBytes(byte);
                strcpy(charHexadecimal, "0x");
            }

            charHexadecimal = strcat(charHexadecimal, byteHexadecimal);
            dividedWordOffset++;
            remainingBytes--;

            if (remainingBytes == 0) {

                // If retrieveIndexFromHexadecimal returns 6, the character is separation or punctuation
                if (retrieveIndexFromHexadecimal(charHexadecimal) == 6) {
                    dividedWordOffset = 0;
                };

                strcpy(charHexadecimal,"");

            }

            chunkData->chunk[byteIndex++] = byte;  

        }

        if (!chunkData->isFinished) {

            chunkData->chunkSize = CHUNK_BYTE_LIMIT - dividedWordOffset;

            /* Remove word offset bytes from the chunk */
            for (int i = 1; i <= dividedWordOffset; i++) {

                chunkData->chunk[CHUNK_BYTE_LIMIT - i] = '\0';
            }

            /* Fall back pointer to the position of the last punctuation/separation character */
            fseek((files + currentFileIndex)->fp, -dividedWordOffset, SEEK_CUR);
        
        }


    }

    /* Mutex unlock */ 
    if ((pthread_mutex_unlock(&count_mutex)) != 0) {
        printf("[ERROR] Can't leave the mutex critical zone.");
        pthread_exit(0);
    }

}





/**
 * @brief 
 * 
 * @param chunkData 
 */
void resetChunkData(struct fileChunk *chunkData) {
    
    chunkData->chunkSize = CHUNK_BYTE_LIMIT;
    chunkData->chunk = (unsigned char *) malloc(CHUNK_BYTE_LIMIT * sizeof(unsigned char));
    chunkData->fileIndex = 0;
    chunkData->numWords = 0;
    for (int i = 0; i < 6; i++) {
        chunkData->nWordsWithVowel[i] = 0;
    }
    chunkData->isFinished = false;

}



/**
 * @brief Get the Results object
 * 
 */
void getResults() {

    for (int i = 0; i < numFiles; i++) {


        printf("ANALYSING FILE: %s\n", (files + i)->filename);
        printf("Total number of words: %d\n", (files + i)->numWords);
        printf("Number of words with an\n");

        printf("%10c %10c %10c %10c %10c %10c\n", 'A', 'E', 'I', 'O', 'U', 'Y');
        printf("%10u %10u %10u %10u %10u %10u\n\n\n", (files + i)->nWordsWithVowel[0],(files + i)->nWordsWithVowel[1],(files + i)->nWordsWithVowel[2],(files + i)->nWordsWithVowel[3],(files + i)->nWordsWithVowel[4],(files + i)->nWordsWithVowel[5]);


    }

}