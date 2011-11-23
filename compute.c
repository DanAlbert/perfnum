/**
 * @file compute.c
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
 * Computes perfect numbers. It tests all numbers beginning from its starting point,
 * subject to the constraints below. There may be more than one copy of compute running
 * simultaneously.
 *
 */
#include <sys/mman.h>
#include <sys/stat.h>
#include <limits.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "shmem.h"

/// The maximum number of divisors to store
#define MAX_DIVISORS 10000

/// Number of arguments to be supplied for pipe method
#define PIPE_ARGC 4

/**
 * @brief Checks if an integer is a perfect number.
 *
 * Preconditions: 
 *
 * Postconditions: 
 *
 * @param n Number to test
 * @return true if n is a perfect number, false otherwise
 */
bool is_perfect_number(unsigned int n);

bool shmem_init(struct shmem_res *res);

bool shmem_loop(struct shmem_res *res);

void pipe_loop(int start, int end);

/**
 * @brief Reports perfect numbers over pipes.
 *
 * Preconditions: stdout is being piped to manage
 *
 * Postconditions: n has been written to pipe
 *
 * @param n Number to report
 */
void pipe_report(int n);

/**
 * @brief Exits the program cleanly.
 *
 * Preconditions:
 *
 * Postconditions: All open resources have been released
 *
 * @param sig The signal which exited the program
 */
void quit(int sig);

int main(int argc, char **argv) {
	struct shmem_res res;
	struct sigaction sigact;
	char mode;
	int start;
	int end;
	
	if (argc < 2) {
		printf("No mode specified");
		exit(EXIT_FAILURE);
	}

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = quit;

	if (sigaction(SIGQUIT, &sigact, NULL) == -1) {
		perror("Could not set SIGQUIT");
	}

	if (sigaction(SIGHUP, &sigact, NULL) == -1) {
		perror("Could not set SIGHUP");
	}

	if (sigaction(SIGINT, &sigact, NULL) == -1) {
		perror("Could not set SIGINT");
	}

	mode = argv[1][0]; // The first character is the mode

	switch (mode) {
	case 'm':
		if (shmem_init(&res) == false) {
			exit(EXIT_FAILURE);
		}
		printf("shmem successful\n");
		shmem_loop(&res);
		break;
	case 'p':
		if (argc < PIPE_ARGC) {
			printf("Test limits not specified.\n");
			exit(EXIT_FAILURE);
		}
		start = atoi(argv[2]);
		end = atoi(argv[3]);
		pipe_loop(start, end);
		break;
	}

	return 0;
}

bool is_perfect_number(unsigned int n) {
	unsigned int divisors[MAX_DIVISORS];
	unsigned int n_divisors = 0;
	unsigned int sum = 0;

	for (unsigned int i = 1; i < n; i++) {
		if ((n % i) == 0) {
			// Is a divisor
			divisors[n_divisors++] = i;
		}
	}

	for (unsigned int i = 0; i < n_divisors; i++) {
		sum += divisors[i];
	}

	return (sum == n);
}

bool shmem_init(struct shmem_res *res) {
    int shmem_fd;
    int bitmap_size;
    int perfnums_size;
    int processes_size;
    int total_size;
    int limit;
    void *addr;

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

bool shmem_loop(struct shmem_res *res) {
	return false;
}

void pipe_loop(int start, int end) {
	for (unsigned int i = start; i <= end; i++) {
			if (is_perfect_number(i) == true) {
				pipe_report(i);
			}
		}
}

void pipe_report(int n) {
	printf("%d\n", n);
}

void quit(int sig) {
	fprintf(stderr, "\rClosing\n");
	close(STDOUT_FILENO);
	exit(sig);
}

