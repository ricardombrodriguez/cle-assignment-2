/**
 *  @file utils.h (interface file)
 *
 *  @brief Interface for important program methods
 *
 *  @author Pedro Sobral & Ricardo Rodriguez
 */
#ifndef UTILS_H
# define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Merges sub-arrays in a bitonic sequence.
 * 
 * @param array Array containing the elements to be merged.
 * @param low Starting index of the sub-array to be merged.
 * @param count Number of elements in the sub-array to be merged.
 * @param direction The direction of sorting (1 for ascending, 0 for descending).
 */
extern void bitonicMerge(int *array, int low, int count, int direction);

/**
 * @brief Recursively sorts a bitonic sequence.
 * 
 * @param array Array containing the elements to be sorted.
 * @param low Starting index of the sub-array to be sorted.
 * @param count Number of elements in the sub-array to be sorted.
 * @param direction The direction of sorting (1 for ascending, 0 for descending).
 */
extern void bitonicMergeSort(int *array, int low, int count, int direction);

/**
 * @brief Validates if the array is sorted in ascending order.
 * 
 * @param array Array to be validated.
 * @param numValues Number of elements in the array.
 * @return 1 if the array is sorted, 0 otherwise.
 */
extern int validation(int *array, int n);



#endif /* UTILS_H */