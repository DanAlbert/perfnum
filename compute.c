/**
 * @file compute.c
 * @author Dan Albert
 * @date Created 11/05/2011
 * @date Last updated 12/05/2011
 * @version 1.0
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
#include <string.h>
#include <unistd.h>
#include "packets.h"
#include "shmem.h"
#include "sock.h"

/// Minimum number of arguments this program needs to run
#define ARGC_MIN 2

/// Number of arguments to be supplied for pipe method
#define PIPE_ARGC 4

/// Number of arguments required for sockets method
#define SOCK_ARGC 3

/// Index of mode argument in argv
#define MODE_ARG 1

/// Index of start argument in argv
#define START_ARG 2

/// Index of end argument in argv
#define END_ARG 3

/// Index of address argument in argv
#define ADDR_ARG 2

/// The maximum number of divisors to store
#define MAX_DIVISORS 10000

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

/**
 * @brief Funds and claims a number for testing
 *
 * Scans through shared memory object for an untested number and claims it.
 *
 * Preconditions: res is not NULL, shared memory initialized
 *
 * Postconditions: A number has been selected or all numbers have been tested
 *
 * @param res Pointer to shared memory resource structure
 * @return Number to test or -1 if all numbers have been tested
 */
int next_test(struct shmem_res *res);

/**
 * @brief Main loop for shared memory
 *
 * Places process in list, then loops, finding numbers to test and reporting
 * them, removing the process from the list upon completion.
 *
 * Preconditions: res is not NULL, shared memory is initialized, there is room
 * in the process list for another process
 *
 * Postconditions: The process has been removed from the process list
 *
 * @param res Pointer to shared memory resource structure
 */
void shmem_loop(struct shmem_res *res);

/**
 * @brief Reports perfect number to shared memory object
 *
 * Preconditions: res is not NULL, shared memory is initialized, n is positive,
 * there is room for another number in the list
 *
 * Postconditions: The number has been placed in the perfect numbers list
 *
 * @param res Pointer to shared memory resource structure
 * @param n Number to report
 * @return true on success, false otherwise
 */
bool shmem_report(struct shmem_res *res, int n);

/**
 * @brief Checks each number in assigned range, reporting when appropriate
 *
 * Preconditions: start is positive, end is greater than start
 *
 * Postconditions: Each number in the range has been tested and reported as
 * necessary
 *
 * @param start First number to test
 * @param end Last number to test
 */
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
 * @brief Cleans up pipe resources
 *
 * Preconditions:
 *
 * Postconditions: Pipe resources have been released
 */
void pipe_cleanup(void);

/**
 * @brief Initializes socket resources
 *
 * Preconditions: Proper arguments have been supplied to the program
 *
 * Postconditions: Socket resources have been initialized and connected
 *
 * @param argc Number of arguments supplied to program
 * @param argv List of arguments supplied to the program
 * @return Socket file descriptor or -1 on error
 */
int sock_init(int argc, char **argv);

/**
 * @brief Checks for perfect numbers
 *
 * Checks assigned range for perfect numbers, requesting a new range as
 * necessary.
 *
 * Preconditions: Sockets have been initialized
 *
 * Postconditions:
 *
 * @param fd Socket file descriptor
 */
void sock_loop(int fd);

/**
 * @brief Reports a perfect number to the managing server
 *
 * Preconditions: Sockets have been initialized
 *
 * Postconditions: The number has been sent to the managing server
 *
 * @param fd Socket file descriptor
 * @param n Number to report
 */
void sock_report(int fd, int n);

/**
 * @brief Cleans up socket resources
 *
 * Preconditions:
 *
 * Postconditions: Socket resources have been released
 *
 * @param fd Socket file descriptor
 */
void sock_cleanup(int fd);

/**
 * @brief Exits the program cleanly.
 *
 * Preconditions:
 *
 * Postconditions: All open resources have been released
 *
 * @param sig The signal which exited the program
 */
void handle_signal(int sig);

/**
 * @brief Displays usage information and exits
 *
 * Preconditions:
 *
 * Postconditions:
 */
void usage(void);

/// Global variable to record caught signal so main loop can exit cleanly
volatile sig_atomic_t exit_status = EXIT_SUCCESS;

/**
 * @brief Entry point for the program
 *
 * Parses arguments for program mode and responds appropriately.
 *
 * Preconditions: Proper arguments have been supplied
 *
 * Postconditions:
 *
 * @param argc Number of arguments supplied
 * @param argv List of arguments supplied
 * @return Exit status
 */
int main(int argc, char **argv) {
	struct shmem_res res;
	struct sigaction sigact;
	char mode;
	int fd;
	int start;
	int end;
	
	if (argc < ARGC_MIN) {
		usage();
	}

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = handle_signal;

	if (sigaction(SIGQUIT, &sigact, NULL) == -1) {
		perror("Could not set SIGQUIT");
	}

	if (sigaction(SIGHUP, &sigact, NULL) == -1) {
		perror("Could not set SIGHUP");
	}

	if (sigaction(SIGINT, &sigact, NULL) == -1) {
		perror("Could not set SIGINT");
	}

	sigact.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &sigact, NULL) == -1) {
		perror("Could not set SIGPIPE handler");
	}

	mode = argv[MODE_ARG][0]; // The first character is the mode

	switch (mode) {
	case 'm':
		if (shmem_load(&res) == false) {
			exit(EXIT_FAILURE);
		}
		shmem_loop(&res);
		break;
	case 'p':
		if (argc < PIPE_ARGC) {
			usage();
		}
		start = atoi(argv[START_ARG]);
		end = atoi(argv[END_ARG]);
		pipe_loop(start, end);
		break;
	case 's':
		fd = sock_init(argc, argv);
		if (fd == -1) {
			exit(EXIT_FAILURE);
		}
		sock_loop(fd);
		sock_cleanup(fd);
		break;
	default:
		usage();
		break;
	}

	exit(exit_status);
}

bool is_perfect_number(unsigned int n) {
	unsigned int divisors[MAX_DIVISORS];
	unsigned int n_divisors = 0;
	unsigned int sum = 0;
	unsigned int i;

	for (i = 1; i < n; i++) {
		if ((n % i) == 0) {
			// Is a divisor
			divisors[n_divisors++] = i;
		}
	}

	for (i = 0; i < n_divisors; i++) {
		sum += divisors[i];
	}

	return (sum == n);
}

int next_test(struct shmem_res *res) {
	int test;
	uint8_t *addr;
	int i;

	assert(res != NULL);

	// Loop over each byte in the bitmap
	// Will actually test until the end of the byte if manage was given a limit
	// that was not a power of two
	for (addr = res->bitmap; addr < (uint8_t *)res->perfect_numbers; addr++) {
		for (i = 0; i < 8; i++) {
			if (BIT(*addr, i) == 0) {

				while (sem_wait(res->bitmap_sem) != 0) {
					if ((errno == EDEADLK) || (errno == EINVAL)) {
						perror("Could not lock semaphore");
						return -1;
					}

					// Else we received EAGAIN or EINTR and should wait again
				}

				// Check to make sure the process that had the semaphore
				// locked didn't claim this number
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
	struct process *p;
	int test;
	bool set = false;

	assert(res != NULL);

	for (p = res->processes; p < (struct process *)res->end; p++) {
		if (p->pid == -1) {
			p->pid = getpid();
			p->found = 0;
			p->tested = 0;

			set = true;
			break;
		}
	}

	if (set == false) {
		fprintf(stderr, "Too many processes already\n");
		return;
	}

	// Claim a new number until all have been tested
	test = next_test(res);
	while (test != -1) {
		if (is_perfect_number(test) == true) {
			p->found++;
			if (shmem_report(res, test) == false) {
				fprintf(stderr, "Could not report perfect number (%d)\n", test);
			}
		}

		p->tested++;

		// Check to see if a signal was caught
		if (exit_status != EXIT_SUCCESS) {
			fputs("\r", stderr);
			break;
		}
		test = next_test(res);
	}

	// Remove self from process list
	p->pid = -1;
}

bool shmem_report(struct shmem_res *res, int n) {
	int i;

	assert(res != NULL);

	while (sem_wait(res->perfect_numbers_sem) != 0) {
		if ((errno == EDEADLK) || (errno == EINVAL)) {
			perror("Could not lock semaphore");
			return false;
		}

		// Else we received EAGAIN or EINTR and should wait again
	}

	for (i = 0; i < NPERFNUMS; i++) {
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
	union packet p;
	int i;

	assert(start > 0);
	assert(end > start);

	for (i = start; i <= end; i++) {
		// Check to see if a signal was caught
		if (exit_status != EXIT_SUCCESS) {
			p.id = PACKETID_CLOSED;
			p.closed.pid = getpid();
			send_packet(STDOUT_FILENO, &p);
			break;
		}

		if (is_perfect_number(i) == true) {
			pipe_report(i);
		}
	}

	if (exit_status == EXIT_SUCCESS) {
		p.id = PACKETID_DONE;
		p.done.pid = getpid();
		send_packet(STDOUT_FILENO, &p);
	}
}

void pipe_report(int n) {
	union packet p;

	p.id = PACKETID_PERFNUM;
	p.perfnum.perfnum = n;

	send_packet(STDOUT_FILENO, &p);
}

void pipe_cleanup(void) {
	close(STDOUT_FILENO);
}

int sock_init(int argc, char **argv) {
	int fd;

	if (argc < SOCK_ARGC) {
		usage();
		return -1;
	}

	fd = sock_connect(argv[ADDR_ARG]);
	if (fd == -1) {
		return -1;
	}

	return fd;
}

void sock_loop(int fd) {
	union packet p;
	bool done = false;
	int i;

	while (done == false) {
		// Check to see if a signal was caught
		if (exit_status != EXIT_SUCCESS) {
			fputs("\r", stderr);
			break;
		}

		p.id = PACKETID_DONE;
		send_packet(fd, &p);

		get_packet(fd, &p);

		switch (p.id) {
		case PACKETID_CLOSED:
			printf("The server has closed the connection\n");
			done = true;
			break;
		case PACKETID_REFUSE:
			done = true;
			break;
		case PACKETID_RANGE:
			for (i = p.range.start; i <= p.range.end; i++) {
				// Check to see if a signal was caught
				if (exit_status != EXIT_SUCCESS) {
					fputs("\r", stderr);
					p.id = PACKETID_CLOSED;
					p.closed.pid = PID_CLIENT;
					send_packet(fd, &p);
					break;
				}
				if (is_perfect_number(i) == true) {
					sock_report(fd, i);
				}
			}
			break;
		default:
			break;
		}
	}
}

void sock_report(int fd, int n) {
	union packet p;

	p.id = PACKETID_PERFNUM;
	p.perfnum.perfnum = n;

	send_packet(fd, &p);
}

void sock_cleanup(int fd) {
	close(fd);
}

void handle_signal(int sig) {
	exit_status = sig;
}

void usage(void) {
	printf("Usage: compute ms <options>\n");
	printf("\n");
	printf("Modes:\n");
	printf("    m - shared memory\n");
	printf("        usage: compute m\n");
	printf("\n");
	printf("    s - sockets\n");
	printf("        usage: compute s <address>\n");
	printf("\n");
	printf("        address:    IP address of managing server\n");
	printf("\n");
	printf("    Note:   The pipes mode can not be spawned directly.\n");
	printf("            Use manage to start pipe mode.\n");
	printf("\n");
	exit(EXIT_FAILURE);
}

