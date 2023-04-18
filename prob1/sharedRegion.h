/**
 *  \authors Pedro Sobral & Ricardo Rodriguez
 *  
 *  Header files containing constants for the main problem.
 *
 */


#ifndef SHAREDREGION_H
#define SHAREDREGION_H

#include <stdbool.h>
#include <stdio.h>

struct file {
    FILE *fp;
    char *filename;
    unsigned int numWords;
    unsigned int nWordsWithVowel[6];
};


struct fileChunk {
    unsigned char *chunk;
    unsigned int fileIndex;
    unsigned int chunkSize;
    unsigned int numWords;
    unsigned int nWordsWithVowel[6];
    bool isFinished;
};

struct fileInfo {
    char *filename;
    FILE *fp;
    bool isFinished;
    int previousChar;
    unsigned char *chunk;
    int chunkSize;
    int nWords;
};

/**
 * @brief
 *
 * @param filenames
 */
extern void
storeFilenames(char *filenames[]);

/**
 * @brief Resets the file structure for the next iteration of the program (new number of threads)
 * 
 */
void resetFilesData();


/**
 * @brief 
 * 
 * @param chunkData 
 * @param threadID 
 */
extern void saveThreadResults(struct fileChunk *chunkData, unsigned int threadID);


/**
 * @brief Get the Chunk object
 * 
 * @param chunkData 
 * @param threadID 
 */
extern void getChunk(struct fileChunk *chunkData, unsigned int threadID);


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


#endif /* SHAREDREGION_H */