/**
 *  \authors Pedro Sobral & Ricardo Rodriguez
 *  
 *  Header files containing constants for the main problem.
 *
 */


#ifndef CONSTANTS_H
#define CONSTANTS_H


/** \brief maximum number of files allowed */
#define MAX_NUM_FILES 10

/** \brief maximum number of files allowed */
#define MAX_NUM_THREADS 8

/** \brief indicates if all files have been processed*/
#define ALL_FILES_PROCESSED 0

/** \brief indicates there are files to be processed*/
#define FILES_TO_BE_PROCESSED 1

/** \brief MPI tag to identify a chunk request */
#define MPI_TAG_CHUNK_REQUEST 1

/** \brief MPI tag to identify a message with partial results from a chunk */
#define MPI_TAG_SEND_RESULTS 2

/** \brief MPI tag to inform the work is done */
#define MPI_TAG_END_WORK 3


#endif /* CONSTANTS_H */