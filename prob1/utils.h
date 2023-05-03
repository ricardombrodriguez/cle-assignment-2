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
 * @param filenames 
 */
extern void storeFilenames(struct fileInfo *files, char *filenames[]);

/**
 * @brief Resets the file structure for the next iteration of the program (new number of threads)
 * 
 */
void resetFilesData();



/**
 * @brief Get the Chunk object
 * 
 * @param chunkData 
 * @param threadID 
 */
extern unsigned int getChunk(struct fileChunk *chunkData, int rank);


/**
 * @brief 
 * 
 * @param chunkData 
 */
extern void resetChunkData(struct fileChunk *chunkData);

/**
 * @brief Get the Results object
 * 
 */
extern void getResults();



extern int getRemainingBytes(int byte);

/**
 * @brief 
 * 
 * @param chunk 
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