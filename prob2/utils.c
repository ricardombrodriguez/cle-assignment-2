
#include <stdlib.h>
#include <math.h>

#include "sharedRegion.h"
#include "sharedRegion.c"

/**
 * @brief Resets the file structure for the next iteration of the program (new number of threads)
 *
 */


extern struct file *files;

extern int currentFileIndex;

void resetFilesData() {
    files = (struct file *){0};
}


/**
 * @brief
 *
 * @param chunkData
 */
void resetChunkData(struct chunk *chunkData) {
    chunkData->chunkList = (int *)malloc((files + currentFileIndex)->chunkSize * sizeof(int));
}


void bitonic_merge(int arr[], int low, int cnt, int dir) {
    if (cnt > 1) {
        int k = cnt / 2;
        for (int i = low; i < low + k; i++) {
            if (1 == (arr[i] > arr[i + k])) {
                int temp = arr[i];
                arr[i] = arr[i + k];
                arr[i + k] = temp;
            }
        }
        bitonic_merge(arr, low, k, dir);
        bitonic_merge(arr, low + k, k, dir);
    }
}

void bitonic_sort_recursive(int arr[], int low, int cnt, int dir) {
    if (cnt > 1) {
        int k = cnt / 2;
        bitonic_sort_recursive(arr, low, k, 1);
        bitonic_sort_recursive(arr, low + k, k, 0);
        bitonic_merge(arr, low, cnt, dir);
    }
}

void bitonic_sort(int arr[], int n) {
    int cnt = 2;
    while (cnt <= n) {
        for (int i = 0; i < n; i += cnt) {
            bitonic_sort_recursive(arr, i, cnt, 1);
        }
        cnt *= 2;
    }
}

void merge_sorted_arrays(int *arr1, int n1, int *arr2, int n2, int *result) {
    int i = 0, j = 0, k = 0;

    while (i < n1 && j < n2) {
        if (arr1[i] <= arr2[j]) {
            result[k++] = arr1[i++];
        } else {
            result[k++] = arr2[j++];
        }
    }

    while (i < n1) {
        result[k++] = arr1[i++];
    }

    while (j < n2) {
        result[k++] = arr2[j++];
    }

    //int *result = (int *)malloc((n1 + n2) * sizeof(int));

    //merge_sorted_arrays(arr1, n1, arr2, n2, result);
}

