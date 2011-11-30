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
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "packets.h"
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

int next_test(struct shmem_res *res);

void shmem_loop(struct shmem_res *res);
bool shmem_report(struct shmem_res *res, int n);

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
		break;fprintf(stderr, "Found perfnum\n");
	case 'p':
		if (argc < PIPE_ARGC) {
			printf("Test limits not specified.\n");
			exit(EXIT_FAILURE);
		}
		start = atoi(argv[2]);
		end = atoi(argv[3]);
		pipe_loop(start, end);
		break;
	default:
		fprintf(stderr, "Unknown mode\n");
		exit(EXIT_FAILURE);
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

				while (sem_wait(res->bitmap_sem) != 0) {
					if ((errno == EDEADLK) || (errno == EINVAL)) {
						perror("Could not lock semaphore");
						return -1;
					}

					// Else we received EAGAIN or EINTR and should wait again
				}

				// Check to make sure the process that had the semaphore locked didn't
				// claim this number
				if (BIT(*addr, i) == 0) {
					// Claim this number for testing
					SET_BIT(*addr, i);

					test = ((addr - res->bitmap) * 8) + i + 1;

					if (sem_post(res->bitmap_sem) == -1) {
						perror("Could not unlock semaphore");
						return false;
					}

					return test;
				} else {
					// Else unlock the semaphore and continue the loop
					if (sem_post(res->bitmap_sem) == -1) {
						perror("Could not unlock semaphore");
						return false;
					}
				}
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
			if (shmem_report(res, test) == false) {
				fprintf(stderr, "Could not report perfect number (%d)\n", test);
			}
		}

		test = next_test(res);
	}
}

bool shmem_report(struct shmem_res *res, int n) {
	assert(res != NULL);

	while (sem_wait(res->perfect_numbers_sem) != 0) {
		if ((errno == EDEADLK) || (errno == EINVAL)) {
			perror("Could not lock semaphore");
			return false;
		}

		// Else we received EAGAIN or EINTR and should wait again
	}

	for (int i = 0; i < NPERFNUMS; i++) {
		if (res->perfect_numbers[i] == 0) {
			// Open slot, use it
			res->perfect_numbers[i] = n;

			if (sem_post(res->perfect_numbers_sem) == -1) {
				perror("Could not unlock semaphore");
				return false;
			}

			return true;
		}
	}

	return false;
}

void pipe_loop(int start, int end) {
	union packet packet;

	assert(start > 0);
	assert(end > start);

	for (int i = start; i <= end; i++) {
		if (is_perfect_number(i) == true) {
			pipe_report(i);
		}
	}

	packet.id = PACKETID_DONE;
	send_packet(STDOUT_FILENO, &packet);
}

void pipe_report(int n) {
	union packet packet;

	packet.id = PACKETID_PERFNUM;
	packet.perfnum.perfnum = n;

	send_packet(STDOUT_FILENO, &packet);
}

void quit(int sig) {
	fprintf(stderr, "\rClosing\n");
	close(STDOUT_FILENO);
	exit(sig);
}

