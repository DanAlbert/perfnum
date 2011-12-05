/**
 * @file shmem.c
 * @author Dan Albert
 * @date Created 11/23/2011
 * @date Last updated 12/05/2011
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * Implements functions for use with the shared memory method.
 *
 */
#include <sys/mman.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include "shmem.h"

bool shmem_load(struct shmem_res *res) {
	int shmem_fd;
	int bitmap_size;
	int perfnums_size;
	int processes_size;
	int total_size;
	int limit;
	void *addr;

	assert(res != NULL);

	// Open the shared memory object
	shmem_fd = shm_open(SHMEM_PATH, O_RDWR, S_IRUSR | S_IWUSR);
	if (shmem_fd == -1) {
		perror("failed to open shared memory object");
		return false;
	}

	if (read(shmem_fd, &limit, sizeof(int)) == -1) {
		perror("Could not read limit");
		return false;
	}

	bitmap_size = limit / 8 + 1;
	perfnums_size = NPERFNUMS * sizeof(int);
	processes_size = NPROCS * sizeof(struct process);
	total_size = sizeof(pid_t) + sizeof(int) + (2 * sizeof(sem_t)) + bitmap_size +
		perfnums_size + processes_size;

	// Check that the size of the shared memory object is the correct size
	if (total_size != lseek(shmem_fd, 0, SEEK_END)) {
		fprintf(stderr, "Shared memory object is invalid\n");
		return false;
	}

	lseek(shmem_fd, 0, SEEK_SET); // Seek start

	addr = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmem_fd, 0);
	if (addr == MAP_FAILED) {
		perror("failed to map shared memory object");
		return false;
	}

	res->addr = addr;
	res->limit = res->addr;
	res->manage = res->limit + 1;
	res->bitmap_sem = (sem_t *)(res->manage + 1);
	
	// limit is a single integer, so bitmap is one int past
	res->bitmap = (uint8_t *)(res->bitmap_sem + 1);
	res->perfect_numbers_sem = (sem_t *)(res->bitmap + bitmap_size);
	res->perfect_numbers = (int *)(res->perfect_numbers_sem + 1);
	res->processes = (struct process *)(res->perfect_numbers + NPERFNUMS);
	res->end = res->processes + NPROCS;

	return true;
}

