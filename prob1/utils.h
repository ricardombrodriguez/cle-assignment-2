/**
 *  \authors Pedro Sobral & Ricardo Rodriguez
 *  
 *  Header files containing constants for the main problem.
 *
 */

#include <stdlib.h>
#include <stdio.h>

#ifndef UTILS_H
#define UTILS_H

/**
 * @brief Structure that saves the global results of each file (number of words and number of words with the vowels [aeiouy] )
 * 
 */
struct fileInfo {
    char *filename;
    FILE *fp;
    unsigned int numWords;
    unsigned int nWordsWithVowel[6];
    bool isFinished;
};

/**
 * @brief Structure that saves the partial results of a chunk, whose data is going to be returned and added to the fileInfo structure (global results) when finished
 * 
 */
struct fileChunk {
    unsigned int fileIndex;
    unsigned char *chunk;
    unsigned int chunkSize;
    unsigned int numWords;
    unsigned int nWordsWithVowel[6];
    bool isFinished;
};


/**
 * @brief Get the text file names by processing the command line and storing them in the shared region for future retrieval by worker threads
 * 
 * @param files fileInfo structure to store data
 * @param filenames name of the input files
 */
extern void storeFilenames(struct fileInfo *files, char *filenames[]);


/**
 * @brief Get the Chunk object of the current file we're reading
 * Byte for byte reading of the file until the chunk reached the CHUNK_BYTE_LIMIT
 * Finishes if CHUNK_BYTE_LIMIT is reached or if we reach the EOF
 * The function removes word offset bytes from the chunk so that the next chunk can
 * fully read the word
 * 
 * 
 * @param chunkData  fileChunk structure
 * @param rank  of the MPI process
 */
extern unsigned int getChunk(struct fileChunk *chunkData, int rank);


/**
 * @brief Resets the fileChunk structure
 * 
 * @param chunkData 
 */
extern void resetChunkData(struct fileChunk *chunkData);

/**
 * @brief Get the Results object
 * 
 */
extern void getResults();


/**
 * @brief Get the Remaining Bytes according to the first byte. Used in the getChunk() method
 * 
 * @param byte 
 * @return int 
 */
extern int getRemainingBytes(int byte);

/**
 * @brief Reads the chunkData->chunkSize bytes belonging to chunkData->chunk and
 * calculates the number of words (numWords) and the number of words with vowels
 * (nWordsWithVowel) of the given chunk
 * 
 * @param chunkData 
 */
extern void processChunk(struct fileChunk *chunkData);


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
extern int retrieveIndexFromHexadecimal(char* hex);

/**
 * @brief Resets all wordHasVowels variable to zero
 * 
 * @param wordHasVowels array of booleans to know if a certain vowel (specified by the index) was already processed in the current word
 * @return void 
 */
extern void resetWordVowels(bool wordHasVowels[6]);


#endif /* UTILS_H */