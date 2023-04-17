/**
 *  @file utils.h (interface file)
 *
 *  @brief 
 *
 *  @author Pedro Sobral & Ricardo Rodriguez, March 2023
 */
#ifndef UTILS_H
# define UTILS_H

/** \brief get the determinant of given matrix */
extern double getDeterminant(int order, double *matrix); 

extern void bitonic_merge(int arr[], int low, int cnt, int dir);

extern void bitonic_sort_recursive(int arr[], int low, int cnt, int dir);

extern void bitonic_sort(int arr[], int n);

extern void merge_sorted_arrays(int *arr1, int n1, int *arr2, int n2, int *result);


#endif /* UTILS_H */