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

/** \brief MPI tag to identify if the program is done (no more tasks to do) */
#define MPI_TAG_PROGRAM_STATE 0

/** \brief MPI tag to identify a chunk request */
#define MPI_TAG_CHUNK_REQUEST 1

/** \brief MPI tag to identify a message with partial results from a chunk */
#define MPI_TAG_SEND_RESULTS 2

#endif /* CONSTANTS_H */