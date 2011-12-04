/**
 * @file report.c
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

bool check_kill(int argc, char **argv, char mode);

int pipe_init(pid_t *manage);
void pipe_report(int fd, pid_t manage);
void pipe_cleanup(int fd);
bool pipe_kill(void);

int load_pid_file(char *path);

void shmem_report(struct shmem_res *res);
bool shmem_kill(struct shmem_res *res);

int sock_init(int argc, char **argv);
void sock_report(int fd);
void sock_cleanup(int fd);
bool sock_kill(int fd);

int next_test(struct shmem_res *res);

void handle_signal(int sig);

/// Global variable to record caught signal so main loop can exit cleanly
volatile sig_atomic_t exit_status = EXIT_SUCCESS;

int main(int argc, char **argv) {
	struct sigaction sigact;
	struct shmem_res res;
	int fd;
	pid_t manage;
	char mode;

	if (argc < 2) {
		printf("Mode not supplied\n");
		exit(EXIT_FAILURE);
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
		printf("Invalid mode\n");
		exit(EXIT_FAILURE);
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
	int next;

	assert(res != NULL);

	for (int i = 0; i < NPERFNUMS; i++) {
		if (res->perfect_numbers[i] != 0) {
			printf("%d\n", res->perfect_numbers[i]);
		}
	}

	next = next_test(res);

	if (next == -1) {
		printf("Testing complete\n");
	} else {
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
		printf("Usage: report s <address>\n");
		return -1;
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
