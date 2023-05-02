/**
 * @file utils.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-03-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */


#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "constants.h"

/* Number of files to be processed */
extern int numFiles;

extern int CHUNK_BYTE_LIMIT;

/* Number of threads */
extern int numThreads;

/* Stores the index of the current file being proccessed by the working processes */
extern int currentFileIndex;

/* File sructure initialization */
struct fileInfo *files;

/* Process control variable - used to know if there's still work to be done (or not) */
extern int workStatus;  


/**
 * @brief Get the text file names by processing the command line and storing them in the shared region for future retrieval by worker threads
 * 
 * @param filenames 
 */
void storeFilenames(char *filenames[]) {

    // Allocate memory dynamically using the malloc function to create an array of numFiles elements of struct 'file' type
    files = (struct fileInfo *) malloc(numFiles * sizeof(struct fileInfo));

    // Initialize each element of 'files' of struct file 
    for (int i = 0; i < numFiles; i++) {
        memset((files + i), 0, sizeof(struct fileInfo));
        (files + i)->fp = NULL;
        (files + i)->filename = filenames[i];
    }

}

/**
 * @brief Resets the file structure for the next iteration of the program (new number of threads)
 * 
 */
void resetFilesData() {

    files = (struct fileInfo *){ 0 };

}


/**
 * @brief Get the Chunk object
 * 
 * @param chunkData 
 * @param threadID 
 */
unsigned int getChunk(struct fileChunk *chunkData) {

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
                    printf("||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n");
                    workStatus = ALL_FILES_PROCESSED;
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

    return chunkData->chunkSize;

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


/**
 * @brief Retrieves an index which will be used to identify the character ('a','e','i','o','u','y',<separation/whitespace/punctiation>,<other>)
 * 
 * @param hex Hexadecimal character representation
 * @return 
 *  0 if 'a'
 *  1 if 'e'
 *  2 if 'i'
 *  3 if 'o'
 *  4 if 'u'
 *  5 if 'y'
 *  6 if SEPARATION, WHITESPACE or PUNCTUATION
 * -1 if other character (consonant)
 */
int retrieveIndexFromHexadecimal(char* hex) {

    /**
     * @brief Bi-dimensional array that stores the string literal of different types of characters (vowels and separators/whitespaces/punctuation)
     * 
     */
    const char* arrays[][30] = {
        {"0x41", "0x61", "0xc380", "0xc381", "0xc382", "0xc383", "0xc384", "0xc385", "0xc386", "0xc3a0", "0xc3a1", "0xc3a2", "0xc3a3", "0xc3a4", "0xc3a5", "0xc3a6"},                                                                                       // a
        {"0x45", "0x65", "0xc388", "0xc389", "0xc38a", "0xc38b", "0xc3a8", "0xc3a9", "0xc3aa", "0xc3ab"},                                                                                                                                                   // e
        {"0x49", "0x69", "0xc38c", "0xc38d", "0xc38e", "0xc38f", "0xc3ac", "0xc3ad", "0xc3ae", "0xc2af"},                                                                                                                                                   // i
        {"0x4f", "0x6f", "0xc392", "0xc393", "0xc394", "0xc395", "0xc396", "0xc398", "0xc3b2", "0xc3b3", "0xc3b4", "0xc3b5", "0xc3b6", "0xc3b8"},                                                                                                           // o
        {"0x55", "0x75", "0xc399", "0xc39a", "0xc39b", "0xc39c", "0xc3b9", "0xc3ba", "0xc3bb", "0xc3bc", "0xc3bd"},                                                                                                                                         // u
        {"0x59", "0x79", "0xc39d", "0xc3bd", "0xc3bf"},                                                                                                                                                                                                     // y                                            
        {"0x20", "0x27", "0x60", "0xe28098", "0xe28099", "0x09", "0x0a", "0x0d", "0x22", "0x2d", "0xe2809c", "0xe2809d", "0x5b", "0x5d", "0x28", "0x29", "0x2c", "0x2e", "0x3a", "0x3b", "0x3f", "0x21", "0xe28093", "0xe280a6", "0x5f", "0xc2bb", "0xc2ab", "0xc2a8", "0xe28094", "0xe28093"}      // SEPARATION, WHITESPACE,PUNCTUATION
    };

    const int num_arrays = sizeof(arrays) / sizeof(arrays[0]);
    const int num_elements = sizeof(arrays[0]) / sizeof(arrays[0][0]);

    /**
     * @brief Iterate through the different arrays and, if some string literal matches the hexadecimal argument, return the index of the array
     * 
     */
    for (int i = 0; i < num_arrays; i++) {
        for (int j = 0; j < num_elements; j++) {
            const char *arrayHex = arrays[i][j];
            if (!arrayHex) {
                continue;
            }
            if (strcmp(hex, arrayHex) == 0) {
                return i;
            }
        }
    }

    /* In the case of a consonant */
    return -1;

}


/**
 * @brief Resets all wordHasVowels variable to zero
 * 
 * @param wordHasVowels array of booleans to know if a certain vowel (specified by the index) was already processed in the current word
 * @return void 
 */
void resetWordVowels(bool wordHasVowels[6]) {

    for (int i=0; i < 6; i++) {
        wordHasVowels[i] = false;   
    }

}



int getRemainingBytes(int byte) {
    if (byte < 192) {
        // 0x0XXXXXXX
        return 1;
    } else if(byte >= 192 && byte < 224) {
        // 0x110XXXXX
        return 2;
    } else if(byte >= 224 && byte < 240) {
        // 0x1110XXXX
        return 3;
    } else {
        //0x11110XXX
        return 4;
    }
}

/**
 * @brief 
 * 
 * @param chunk 
 */
void processChunk(struct fileChunk *chunkData) {

    char *charHexadecimal = (char *) malloc(12 * sizeof(char));             /* Byte limit of hexadecimal string */
    strcpy(charHexadecimal, "0x");                                          // Used to store full hexadecimal representation of character
    int remainingBytes = 0;                                                 // Calculates how many bytes are left for the UTF-8 character. If byte 0 is 0xxxxxxx = 0, 110xxxxx = 1, 1110xxxx = 2, etc...
    bool inWord = false;                                                    // Tells if we're still iterating through a word character
    bool wordHasVowels[6] = { 0,0,0,0,0,0 };                                // Array that checks if vowels exist in word

    int byte;

    printf("CHUNKKKKKKKKKKKKKKK SIZEEEEEEEEEEEEEEEE %u\n\n\n", chunkData->chunkSize);

    for (int i = 0; i < chunkData->chunkSize; i++) {

        byte = chunkData->chunk[i];

        /* Get string representation of the last hexadecimal (byte) */
        char lastHexadecimal[9];
        sprintf(lastHexadecimal, "%02x", byte);

        // Check if character has, or not, continuation.
        // If it doesn't have, calculate the character with the retrieve_character_from_byte
        // Else, wait to append the other bytes

        // Read first hexadecimal and check how many bytes are left according to byte 0 (initial)
        if (strcmp(charHexadecimal, "0x") == 0) {

            if (byte < 192) {
                // 0x0XXXXXXX
                remainingBytes = 1;
            } else if(byte >= 192 && byte < 224) {
                // 0x110XXXXX
                remainingBytes = 2;
            } else if(byte >= 224 && byte < 240) {
                // 0x1110XXXX
                remainingBytes = 3;
            } else if(byte >= 240) {
                //0x11110XXX
                remainingBytes = 4;
            }

        }

        charHexadecimal = strcat(charHexadecimal, lastHexadecimal);

        if (remainingBytes > 1) {

            remainingBytes--;

        } else {

            // Last remaining byte, build the hexadecimal representation of the UTF-8 character and process it

            int index = retrieveIndexFromHexadecimal(charHexadecimal);


            if (index >= 0 && index <= 5) {

                // if it's a vowel
                inWord = true;
                
                if (!wordHasVowels[index]) {
                    chunkData->nWordsWithVowel[index] += 1;
                    wordHasVowels[index] = true;
                }

            } else if (index == 6) {

                if ( (strcmp(charHexadecimal, "0x27") != 0) && (strcmp(charHexadecimal, "0x60") != 0) && strcmp(charHexadecimal, "0xe28098") != 0 && strcmp(charHexadecimal, "0xe28099") != 0 ) {

                    if (inWord) {
                        chunkData->numWords += 1;
                        resetWordVowels(wordHasVowels);
                        inWord = false;
                    }
                }

            } else {

                // If it's other character than vowel or whitespaces
                inWord = true;

            }

            strcpy(charHexadecimal,"0x");

        }

    }

    printf("[PROCESS CHUNK] NUm words: %u\n", chunkData->numWords);

}