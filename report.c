/**
 * @file report.c
 * @author Dan Albert
 * @date Created 11/05/2011
 * @date Last updated 11/05/2011
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
 * Reports on the perfect numbers found, the number tested, and the processes currently
 * computing. If invoked with the "-k" switch, it also is used to inform the manage
 * process to shut down computation.
 *
 */
#include <sys/mman.h>
#include <sys/stat.h>
#include <assert.h>
#include <limits.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "shmem.h"

bool shmem_init(struct shmem_res *res);
void shmem_report(struct shmem_res *res);

void pipe_report(void);

int main(int argc, char **argv) {
	struct shmem_res res;
	char mode;

	if (argc < 2) {
		printf("Mode not supplied\n");
		exit(EXIT_FAILURE);
	}

	mode = argv[1][0]; // Mode is first char

	switch (mode) {
	case 'm':
		if (shmem_init(&res) == -1) {
			exit(EXIT_FAILURE);
		}
		shmem_report(&res);
		break;
	case 'p':
		pipe_report();
		break;
	case 's':
		printf("Sockets not implemented\n");
		exit(EXIT_FAILURE);
	default:
		printf("Invalid mode\n");
		break;
	}

	exit(EXIT_SUCCESS);
}

bool shmem_init(struct shmem_res *res) {
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
	total_size = sizeof(int) + bitmap_size + perfnums_size + processes_size;

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
	res->bitmap = res->limit + 1; // limit is a single integer, so bitmap is one int past
	res->perfect_numbers = res->bitmap + bitmap_size;
	res->processes = res->perfect_numbers + NPERFNUMS;

	return true;
}

void shmem_report(struct shmem_res *res) {
	assert(res != NULL);

	for (int i = 0; i < NPERFNUMS; i++) {
		if (res->perfect_numbers[i] != 0) {
			printf("%d\n", res->perfect_numbers[i]);
		}
	}
}

void pipe_report(void) {
	char c = getchar();
	while (c != EOF) {
		putchar(c);
		c = getchar();
	}
}
