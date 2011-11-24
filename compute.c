/**
 * @file compute.c
 * @author Dan Albert
 * @date Created 11/05/2011
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
 * Computes perfect numbers. It tests all numbers beginning from its starting point,
 * subject to the constraints below. There may be more than one copy of compute running
 * simultaneously.
 *
 */
#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "shmem.h"

/// Macro to get the value of a specific bit
#define BIT(byte, bit) ((byte >> bit) & 1)

/// Macro to set a specific bit
#define SET_BIT(byte, bit) (byte |= (1 << bit))

/// Macro to clear a specific bit
#define CLR_BIT(byte, bit) (byte &= ~(1 << bit))

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

int next_test(struct shmem_res *res);

void shmem_loop(struct shmem_res *res);
void shmem_report(struct shmem_res *res, int n);

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
		if (shmem_load(&res) == false) {
			exit(EXIT_FAILURE);
		}
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

int next_test(struct shmem_res *res) {
	int test;

	assert(res != NULL);

	// Loop over each byte in the bitmap
	// Will actually test until the end of the byte if manage was given a limit that was
	// not a power of two
	for (uint8_t *addr = res->bitmap; addr < res->perfect_numbers; addr++) {
		for (int i = 0; i < 8; i++) {
			if (BIT(*addr, i) == 0) {
				// Claim this number for testing
				SET_BIT(*addr, i);

				test = ((addr - res->bitmap) * 8) + i + 1;

				// Technically not the start of the bitmap, but it will speed up the
				// search for the next number to test
				//res->bitmap = addr + 1;

				return test;
			}
		}
	}

	return -1;
}

void shmem_loop(struct shmem_res *res) {
	int test;

	assert(res != NULL);

	test = next_test(res);
	while (test != -1) {
		if (is_perfect_number(test) == true) {
			shmem_report(res, test);
		}

		test = next_test(res);
	}
}

void shmem_report(struct shmem_res *res, int n) {
	assert(res != NULL);

	for (int i = 0; i < NPERFNUMS; i++) {
		if (res->perfect_numbers[i] == 0) {
			// Open slot, use it
			res->perfect_numbers[i] = n;
			return;
		}
	}
}

void pipe_loop(int start, int end) {
	assert(start > 0);
	assert(end > start);

	for (int i = start; i <= end; i++) {
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

