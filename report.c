/**
 * @file report.c
 * @author Dan Albert
 * @date Created 11/05/2011
 * @date Last updated 12/04/2011
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
 * Reports on the perfect numbers found, the number tested, and the processes currently
 * computing. If invoked with the "-k" switch, it also is used to inform the manage
 * process to shut down computation.
 *
 */
#include <assert.h>
#include <errno.h>
#include <fcntl.h> // For O_RDONLY
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "packets.h"
#include "shmem.h"
#include "sock.h"

/// Number of arguments required for sockets method
#define PIPE_ARGC 2

/// Number of arguments required for sockets method
#define SHMEM_ARGC 2

/// Number of arguments required for sockets method
#define SOCK_ARGC 3

/// File path of named pipe for pipe method
#define FIFO_PATH ".perfect_numbers"

/// Pid file PATH
#define PID_FILE "manage.pid"

/// Maximum size of the PID string
#define SPIDSTR 11

/**
 * @brief Checks command line arguments for kill option
 *
 * Preconditions: Valid mode specified
 *
 * Postconditions:
 *
 * @param argc Number of command line arguments
 * @param argv List of command line arguments
 * @param mode Mode specifed at command line
 * @return true if kill option was speciefied, false otherwise
 */
bool check_kill(int argc, char **argv, char mode);

/**
 * @brief Initializes pipe resources
 *
 * Preconditions: manage is not NULL
 *
 * Postconditions: Managing process ID has been loaded, FIFO has been opened
 *
 * @param manage Pointer to memory to load managing process ID into
 * @return FIFO file descriptor or -1 on error
 */
int pipe_init(pid_t *manage);

/**
 * @brief Responds to and reports messages from managign process
 *
 * Preconditions: Pipe resources have been initialized
 *
 * Postconditions:
 *
 * @param fd FIFO file descriptor
 * @param manage Process ID of managing process
 */
void pipe_report(int fd, pid_t manage);

/**
 * @brief Cleans up pipe resources
 *
 * Preconditions:
 *
 * Postconditions: Pipe resources have been released
 *
 * @param fd FIFO file descriptor
 */
void pipe_cleanup(int fd);

/**
 * @brief Signals managing process to shut down computation
 *
 * Preconditions:
 *
 * Postconditions: Managing process has been signaled to shut down computation
 *
 * @return true on success, false otherwise
 */
bool pipe_kill(void);

/**
 * @brief Loads a PID from a file
 *
 * Preconditions: path is not NULL, the file at path is readable, the file at path
 * contains a process ID
 *
 * Postconditions: Process ID has been read
 *
 * @param path Path of the PID file to read
 * @return PID contained in file or -1 on error
 */
int load_pid_file(char *path);

/**
 * @brief Reports perfect numbers and computation statistics
 *
 * Preconditions: res is not NULL, shared memory resources have been initialized
 *
 * Postconditions: Data has been reported
 *
 * @param res Pointer to shared memory resource strucure
 */
void shmem_report(struct shmem_res *res);

/**
 * @brief Signals managing process to shut down computation
 *
 * Preconditions: res is not NULL, shared memory resources have been initialized
 *
 * Postconditions: Managing process has been signaled to shut down computation
 *
 * @return true on success, false otherwise
 */
bool shmem_kill(struct shmem_res *res);

/**
 * @brief Initializes and connects socket resources
 *
 * Preconditions: Appropriate command line arguments have been supplied
 *
 * Postconditions: Socket resources have been initialized, the client has connected and
 * the client is registered with the server to be notified of perfect numbers
 *
 * @param argc Number of command line arguments
 * @param argv List of command line arguments
 * @return Socket file descriptor or -1 on error
 */
int sock_init(int argc, char **argv);

/**
 * @brief Reports received information from server
 *
 * Preconditions: Sockets have been initialized
 *
 * Postconditions: Information has been reported
 *
 * @param fd Socket file descriptor
 */
void sock_report(int fd);

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
 * @brief Signals managing server to shut down computation
 *
 * Preconditions: Sockets have been initialized
 *
 * Postconditions: Managing server has been signaled to shut down computation
 *
 * @param fd Socket file descriptor
 * @return true on success, false otherwise
 */
bool sock_kill(int fd);

/**
 * @brief Finds the next untested number
 *
 * Preconditions: res is not NULL, shared memory has been initialized
 *
 * Postconditions:
 *
 * @param res Pointer to shared memory resource structure
 * @return Next untested number or -1 if all numbers have been tested
 */
int next_test(struct shmem_res *res);

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
	struct sigaction sigact;
	struct shmem_res res;
	int fd;
	pid_t manage;
	char mode;

	if (argc < 2) {
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

	mode = argv[1][0]; // Mode is first char

	switch (mode) {
	case 'm':
		if (shmem_load(&res) == false) {
			exit(EXIT_FAILURE);
		}

		if (check_kill(argc, argv, mode)) {
			if (shmem_kill(&res) == false) {
				exit(EXIT_FAILURE);
			}
		} else {
			shmem_report(&res);
		}
		break;
	case 'p':
		if (check_kill(argc, argv, mode)) {
			if (pipe_kill() == false) {
				exit(EXIT_FAILURE);
			}
		} else {
			fd = pipe_init(&manage);
			if (fd == -1) {
				exit(EXIT_FAILURE);
			}
			pipe_report(fd, manage);
			pipe_cleanup(fd);
		}
		break;
	case 's':
		fd = sock_init(argc, argv);
		if (fd == -1) {
			exit(EXIT_FAILURE);
		}

		if (check_kill(argc, argv, mode)) {
			if (sock_kill(fd) == false) {
				sock_cleanup(fd);
				exit(EXIT_FAILURE);
			}
		} else {
			sock_report(fd);
			sock_cleanup(fd);
		}
		break;
	default:
		usage();
		break;
	}

	exit(EXIT_SUCCESS);
}

bool check_kill(int argc, char **argv, char mode) {
	switch (mode) {
	case 'm':
		if (argc > SHMEM_ARGC) {
			if (strcmp(argv[SHMEM_ARGC], "-k") == 0) {
				return true;
			}
		}
		break;
	case 'p':
		if (argc > PIPE_ARGC) {
			if (strcmp(argv[PIPE_ARGC], "-k") == 0) {
				return true;
			}
		}
		break;
	case 's':
		if (argc > SOCK_ARGC) {
			if (strcmp(argv[SOCK_ARGC], "-k") == 0) {
				return true;
			}
		}
		break;
	}

	return false;
}

int pipe_init(pid_t *manage) {
	int fd;

	*manage = load_pid_file(PID_FILE);
	if (*manage == -1) {
		perror("Could not load pid file");
		return -1;
	}

	fd = open(FIFO_PATH, O_RDONLY);
	if (fd == -1) {
		perror("Could not open FIFO");
		return -1;
	}

	return fd;
}

void pipe_report(int fd, pid_t manage) {
	union packet packet;
	ssize_t chars_read;
	bool done = false;

	while (done == false) {
		// Check to see if a signal was caught
		if (exit_status != EXIT_SUCCESS) {
			fputs("\r", stderr);
			break;
		}

		chars_read = get_packet(fd, &packet);
		if (chars_read == -1) {
			if ((errno != EAGAIN) && (errno != EINTR)) {
				perror("Could not read packet");
			}
		}

		if (chars_read > 0) {
			switch (packet.id) {
			case PACKETID_PERFNUM:
				printf("%d\n", packet.perfnum.perfnum);
				break;
			case PACKETID_DONE:
				printf("Computation complete\n");
				done = true;
				break;
			case PACKETID_CLOSED:
				if (packet.closed.pid == manage) {
					printf("Manage was shut down before execution could complete\n");
					done = true;
				} else {
					printf("A compute process exited prematurely. ");
					printf("Perfect numbers may be missed.\n");
				}
				break;
			case PACKETID_NULL:
			case PACKETID_RANGE:
				printf("Invalid packet: %#02x\n", packet.id);
				break;
			default:
				printf("Unrecognized packet: %#02x\n", packet.id);
				break;
			}
		}
	}
}

void pipe_cleanup(int fd) {
	close(fd);
}

bool pipe_kill(void) {
	pid_t manage;

	// Signal manage to shutdown computation
	manage = load_pid_file(PID_FILE);
	if (manage != -1) {
		if (kill(manage, SIGQUIT) == -1) {
			perror("Could not shut down computation");
			return false;
		}
	} else {
		printf("Managing process not running\n");
		return false;
	}

	return true;
}

int load_pid_file(char *path) {
	char pid_str[SPIDSTR];
	int fd;

	assert(path != NULL);

	fd = open(path, O_RDONLY);
	if (fd == -1) {
		return -1;
	}

	if (read(fd, pid_str, SPIDSTR) == -1) {
		return -1;
	}

	return atoi(pid_str);
}

void shmem_report(struct shmem_res *res) {
	int total = 0;
	int next;
	bool first_proc = true;

	assert(res != NULL);

	printf("Perfect numbers:\n");
	for (int i = 0; i < NPERFNUMS; i++) {
		if (res->perfect_numbers[i] != 0) {
			printf("%d\n", res->perfect_numbers[i]);
		}
	}

	for (struct process *p = res->processes; p < res->end; p++) {
		if (p->pid != -1) {
			if (first_proc == true) {
				printf("\nProcesses:\n");
				first_proc = false;
			}

			printf("compute (%d): tested %d, found %d\n", p->pid, p->tested, p->found);
			total += p->tested;
		}
	}

	next = next_test(res);

	if (next == -1) {
		printf("\nTesting complete\n");
	} else {
		printf("\n%d tested, %d remaining\n", total, *res->limit - total);
		printf("Next untested integer: %d\n", next);
	}
}

bool shmem_kill(struct shmem_res *res) {
	if (kill(*res->manage, SIGQUIT) == -1) {
		perror("Could not kill manage");
		return false;
	}

	return true;
}

int sock_init(int argc, char **argv) {
	union packet p;
	int fd;

	if (argc < SOCK_ARGC) {
		usage();
	}

	fd = sock_connect(argv[2]);
	if (fd == -1) {
		return -1;
	}

	p.id = PACKETID_NOTIFY;
	send_packet(fd, &p);

	get_packet(fd ,&p);
	if (p.id == PACKETID_ACCEPT) {
		return fd;
	} else if (p.id == PACKETID_REFUSE) {
		fprintf(stderr, "A client is already registered to be notified by the server\n");

		// Disconnect
		close(fd);

		return -1;
	} else {
		fprintf(stderr, "Invalid or unknown packet (%d)\n", p.id);

		// Disconnect
		close(fd);

		return -1;
	}
}

void sock_report(int fd) {
	union packet p;
	ssize_t bytes_read;
	bool done = false;

	while (done == false) {
		// Check to see if a signal was caught
		if (exit_status != EXIT_SUCCESS) {
			fputs("\r", stderr);
			break;
		}

		bytes_read = get_packet(fd, &p);
		if (bytes_read > 0) {
			switch (p.id) {
			case PACKETID_PERFNUM:
				printf("%d\n", p.perfnum.perfnum);
				break;
			case PACKETID_DONE:
				printf("Computation complete\n");
				done = true;
				break;
			case PACKETID_CLOSED:
				if (p.closed.pid == PID_SERVER) {
					printf("Manage was shut down before execution could complete\n");
					done = true;
				} else {
					printf("A compute process exited prematurely. ");
					printf("Perfect numbers may be missed.\n");
				}
				break;
			case PACKETID_NULL:
			case PACKETID_RANGE:
			case PACKETID_NOTIFY:
				fprintf(stderr, "Invalid packet: %#02x\n", p.id);
				break;
			default:
				fprintf(stderr, "Unrecognized packet: %#02x\n", p.id);
				break;
			}
		}
	}
}

void sock_cleanup(int fd) {
	close(fd);
}

bool sock_kill(int fd) {
	union packet p;

	p.id = PACKETID_KILL;
	if (send_packet(fd, &p) == -1) {
		perror("Could not kill server");
		return false;
	}

	return true;
}

int next_test(struct shmem_res *res) {
	assert(res != NULL);

	// Loop over each byte in the bitmap
	// Will actually test until the end of the byte if manage was given a limit that was
	// not a power of two
	for (uint8_t *addr = res->bitmap; addr < res->perfect_numbers; addr++) {
		for (int i = 0; i < 8; i++) {
			if (BIT(*addr, i) == 0) {
				return ((addr - res->bitmap) * 8) + i + 1;
			}
		}
	}

	return -1;
}

void handle_signal(int sig) {
	exit_status = sig;
}

void usage(void) {
	printf("Usage: report mps <options>\n");
	printf("\n");
	printf("Modes:\n");
	printf("    m - shared memory\n");
	printf("        usage: report m [-k]\n");
	printf("\n");
	printf("        -k:         shut down computation\n");
	printf("\n");
	printf("    p - pipes\n");
	printf("        usage: report p [-k]\n");
	printf("\n");
	printf("        -k:         shut down computation\n");
	printf("\n");
	printf("    s - sockets\n");
	printf("        usage: report s <address> [-k]\n");
	printf("\n");
	printf("        address:    IP address of managing server\n");
	printf("        -k:         shut down computation\n");
	printf("\n");
	exit(EXIT_FAILURE);
}

