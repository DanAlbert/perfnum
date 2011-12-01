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
#define SOCK_ARGC 3

/// File path of named pipe for pipe method
#define FIFO_PATH ".perfect_numbers"

int pipe_init(void);
void pipe_report(int fd);
void pipe_cleanup(int fd);

void kill_processes(void);

void shmem_report(struct shmem_res *res);

int sock_init(int argc, char **argv);

int next_test(struct shmem_res *res);

int main(int argc, char **argv) {
	struct shmem_res res;
	int fd;
	char mode;

	if (argc < 2) {
		printf("Mode not supplied\n");
		exit(EXIT_FAILURE);
	}

	mode = argv[1][0]; // Mode is first char

	switch (mode) {
	case 'm':
		if (shmem_load(&res) == false) {
			exit(EXIT_FAILURE);
		}
		shmem_report(&res);
		break;
	case 'p':
		fd = pipe_init();
		if (fd == -1) {
			fprintf(stderr, "Could not initialize pipes");
			exit(EXIT_FAILURE);
		}
		pipe_report(fd);
		pipe_cleanup(fd);
		break;
	case 's':
		fd = sock_init(argc, argv);
		if (fd == -1) {
			exit(EXIT_FAILURE);
		}
		sock_report(fd);
		sock_cleanup(fd);
		break;
	case 'k':
		// Signal manage to shutdown computation
		printf("Kill not implemented\n");
		kill_processes();
		exit(EXIT_FAILURE);
	default:
		printf("Invalid mode\n");
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

int pipe_init(void) {
	return open(FIFO_PATH, O_RDONLY);
}

void pipe_report(int fd) {
	union packet packet;
	ssize_t chars_read;
	bool done = false;

	while (done == false) {
		chars_read = get_packet(fd, &packet);
		if (chars_read == 0) {
			//break;
		} else if (chars_read == -1) {
			if (errno != EAGAIN) {
				perror(NULL);
			}
		}

		if (chars_read > 0) {
			switch (packet.id) {
			case PACKETID_PERFNUM:
				printf("%d\n", packet.perfnum.perfnum);
				break;
			case PACKETID_DONE:
				printf("Computation complete. Killing processes...\n");
				kill_processes();
				done = true;
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

void kill_processes(void) {
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
	if (p.id == PACKETID_REFUSE) {
		fprintf(stderr, "A client is already registered to be notified by the server\n");

		// Disconnect
		close(fd);

		return -1;
	} else if (p.id == PACKETID_PERFNUM) {
		printf("%d\n", p.perfnum.perfnum);
	} else {
		fprintf(stderr, "Invalid or unknown packet (%d)\n", p.id);
	}

	return fd;
}

void sock_report(int fd) {
	union packet p;
	ssize_t bytes_read;
	bool done = false;

	while (done == false) {
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
			case PACKETID_NULL:
			case PACKETID_RANGE:
			case PACKETID_NOTIFY:
			case PACKETID_REFUSE:
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

