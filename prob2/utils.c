/**
 *  @file utils.c 
 *
 *  @brief Important methods for bitonic sorting and validation
 *
 *  @author Pedro Sobral & Ricardo Rodriguez
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "utils.h"

/**
* @brief Merges sub-arrays in a bitonic sequence.
*
* @param arr Array containing the elements to be merged.
* @param low Starting index of the sub-array to be merged.
* @param count Number of elements in the sub-array to be merged.
* @param direction The direction of sorting (1 for ascending, 0 for descending).
*/
void bitonicMerge(int *array, int low, int count, int direction)
{
    if (count > 1)
    {
        int k = count / 2;
        for (int i = low; i < low + k; i++)
            {
            if (direction == (array[i] > array[i + k]))
            {
                int temp = array[i];
                array[i] = array[i + k];
                array[i + k] = temp;
            }
        }
        bitonicMerge(array, low, k, direction);
        bitonicMerge(array, low + k, k, direction);
    }
}

/**
*
* @brief Recursively sorts a bitonic sequence.
*
* @param array Array containing the elements to be sorted.
* @param low Starting index of the sub-array to be sorted.
* @param count Number of elements in the sub-array to be sorted.
* @param direction The direction of sorting (1 for ascending, 0 for descending).
*/
void bitonicMergeSort(int *array, int low, int count, int direction)
{
    if (count > 1)
    {
        int k = count / 2;
        bitonicMergeSort(array, low, k, 1);
        bitonicMergeSort(array, low + k, k, 0);
        bitonicMerge(array, low, count, direction);
    }
}

/**
 * @brief Validates if the array is sorted in ascending order.
 * 
 * @param array Array to be validated.
 * @param numValues Number of elements in the array.
 * @return 1 if the array is sorted, 0 otherwise.
 */
int validation(int *array, int numValues)
{
    for (int i = 1; i < numValues; i++)
    {
        if (array[i - 1] > array[i])
        {
            return 0;
        }
    }
    return 1;
}
