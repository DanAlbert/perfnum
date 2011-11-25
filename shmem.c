/**
 * @file shmem.c
 * @author Dan Albert
 * @date Created 11/23/2011
 * @date Last updated 11/23/2011
 * @version 0.1
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
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

    /* create and resize it */
    shmem_fd = shm_open(SHMEM_PATH, O_RDWR, S_IRUSR | S_IWUSR);
    if (shmem_fd == -1){
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
	total_size = sizeof(int) + (2 * sizeof(sem_t)) + bitmap_size + perfnums_size + processes_size;

	// Check that the size of the shared memory object is the correct size
	if (total_size != lseek(shmem_fd, 0, SEEK_END)) {
		fprintf(stderr, "Shared memory object is invalid\n");
		return false;
	}

	lseek(shmem_fd, 0, SEEK_SET); // Seek start

    addr = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmem_fd, 0);
    if (addr == MAP_FAILED){
        perror("failed to map shared memory object");
        return false;
    }

	res->addr = addr;
	res->limit = res->addr;
	res->bitmap_sem = res->limit + 1;
	res->bitmap = res->bitmap_sem + 1; // limit is a single integer, so bitmap is one int past
	res->perfect_numbers_sem = res->bitmap + bitmap_size;
	res->perfect_numbers = res->perfect_numbers_sem + 1;
	res->processes = res->perfect_numbers + NPERFNUMS;

	return true;
}
