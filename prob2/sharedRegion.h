/**
 *  @file sharedregion.h
 *
 *  @brief
 *
 *  @author Pedro Sobral & Ricardo Rodriguez - March 2022
 *
 */

#include <stdbool.h>
#include <stdio.h>

#ifndef SHARED_REGION_H
#define SHARED_REGION_H

/** \brief structure with file information */
struct file {
    FILE *fp;
    char *filename;
    unsigned int numNumbers;
    unsigned int chunkSize; /* Number of integers each working thread will support */
    int *sortedList;        /* Sorted list of the file */
    int numberOfValues;     /* Number of values in the file */
};

/** \brief structure with chunk information */
struct chunk {
    int *chunkList;                 /* Sorted sub-list of the chunk */
    int chunkSize;                  /* Size of the chunk */
    int sortedListOffset;           /* Index/Position of the chunk in the file structure sortedList */
    bool isFinished;                /* Flag to check if the chunk has been processed */
};


/**
 * @brief 
 * 
 * @param filenames 
 * @param numNumbers 
 */
extern void storeFilenames(char *filenames[], unsigned int numNumbers[]);


/**
 * @brief 
 * 
 * @param chunkData 
 * @param threadID 
 */
extern void saveThreadResults(struct chunk *chunkData, unsigned int threadID);


/**
 * @brief Get the Chunk object
 * 
 * @param chunkData 
 * @param threadID 
 */
extern void getChunk(struct chunk *chunkData, unsigned int threadID);


/**
 * @brief Get the Results object
 * 
 */
extern int validation();



#endif /* SHARED_REGION_H */