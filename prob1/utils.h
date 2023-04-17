/**
 *  \authors Pedro Sobral & Ricardo Rodriguez
 *  
 *  Header files containing constants for the main problem.
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "sharedRegion.h"

#ifndef UTILS_H
#define UTILS_H


int getRemainingBytes(int byte);

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