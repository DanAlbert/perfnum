/**
 * @file shmem.h
 * @author Dan Albert
 * @date Created 11/23/2011
 * @date Last updated 12/05/2011
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * Defines structures, constants and functions for use with the shared memory method.
 *
 */
#ifndef SHMEM_H
#define SHMEM_H

#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

/// Macro to get the value of a specific bit
#define BIT(byte, bit) ((byte >> bit) & 1)

/// Macro to set a specific bit
#define SET_BIT(byte, bit) (byte |= (1 << bit))

/// Macro to clear a specific bit
#define CLR_BIT(byte, bit) (byte &= ~(1 << bit))

/// Name of shared memory object
#define SHMEM_PATH "albertd"

/// Maximum number of perfect numbers to store in shared memory
#define NPERFNUMS 20

/// Maximum number of processes to track in shared memory
#define NPROCS 20


/**
 * Process data structure
 */
struct process {
	pid_t pid;	///< Process identifier
	int found;	///< Number of perfect numbers found
	int tested;	///< Number of numbers tested
};


/**
 * Shared memory layout structure
 */
struct shmem_res {
	void *addr;					///< Address of the beginning of the shared memory object
	int *limit;					///< Address of the computation limit
	pid_t *manage;				///< Address of the managing process's ID
	sem_t *bitmap_sem;			///< Address of the bitmap semaphore
	uint8_t *bitmap;			///< Address of the beginning of the bitmap
	sem_t *perfect_numbers_sem;	///< Address of the perfect numbers semaphore
	int *perfect_numbers;		///< Address of the beginning of the perfect numebrs array
	struct process *processes;	///< Address of the beginning of the process list
	void *end;					///< Address of the end of the shared memory object
};


/**
 * @brief Opens and mmaps shared memory object
 *
 * Preconditions: res is not NULL, shared memory object exists and is readable, shared
 * memory object is the appropriate size
 *
 * Postconditions: Shared memory object has been opened and mapped, resource locations
 * have been set in res
 *
 * @param res Pointer to shared memory resource strucure
 * @return true on success, false otherwise
 */
bool shmem_load(struct shmem_res *res);

#endif // SHMEM_H

