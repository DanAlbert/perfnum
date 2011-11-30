/**
 * @file manage.c
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
 * Maintains the results of compute. It also keeps track of the active compute processes,
 * so that it can signal them to terminate.
 *
 */
#include <arpa/inet.h>
#include <netinet/in.h> // For sockaddr_in
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h> // For mkfifo()
#include <sys/time.h> // For timeval
#include <sys/types.h> // For S_IRUSR, etc.
#include <sys/wait.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h> // For PIPE_BUF
#include <math.h> // For floor()
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For memset()
#include <unistd.h>
#include "packets.h"
#include "shmem.h"

/// Path to compute program
#define COMPUTE_CMD "./compute"

/// Path to compute program
#define REPORT_CMD "./report"

/// Number of arguments required for pipe method
#define PIPE_ARGC 4

/// File path of named pipe for pipe method
#define FIFO_PATH ".perfect_numbers"

/// File mode of named pipe for pipe method
#define FIFO_MODE (S_IRUSR | S_IWUSR)

/// Number of arguments required for shared memory method
#define SHMEM_ARGC 3

/// Number of arguments required for sockets method
#define SOCK_ARGC 2

/// Size of the perfnums array in pipe_res
#define SPERFNUMS 5

/// Port the server will listen on
#define SERVER_PORT 10054

/// Maximum number of queued connections
#define MAX_BACKLOG 32

/// Maximum number of clients to allow
#define MAX_CLIENTS FD_SETSIZE

/// Read buffer size
#define BUF_SIZE 8192

#define READ 0
#define WRITE 1

struct pipe_res {
	pid_t *compute_pids;
	int perfnums[SPERFNUMS];
	int nperfnums;
	int compute_pipe[2];
	int report_fifo;
	int nprocs;
	int limit;
};

struct sock_res {
	int listen;
	int clients[MAX_CLIENTS];
	fd_set allfds;
	int maxfd;
	int maxi;
};

bool pipe_init(int argc, char **argv, struct pipe_res *res);
void pipe_report(struct pipe_res *res);
void pipe_cleanup(struct pipe_res *res);

bool shmem_init(int argc, char **argv, struct shmem_res *res);
void shmem_cleanup(void);

bool sock_init(int argc, char **argv, struct sock_res *res);
void sock_report(struct sock_res *res);
void sock_cleanup(struct sock_res *res);

int spawn_computes(pid_t **pids, int fds[2], int limit, int nprocs);
bool collect_zombies(struct pipe_res *res);

void *shmem_mount(char *path, int object_size);

void usage(void);

int main(int argc, char **argv) {
	struct pipe_res pipe_res;
	struct shmem_res shmem_res;
	struct sock_res sock_res;
	char mode;

	if (argc < 2) {
		usage();
	}

	mode = argv[1][0]; // Only need the first character

	switch (mode) {
	case 'p':
		// Pipe stuff
		if (pipe_init(argc, argv, &pipe_res) == false) {
			exit(EXIT_FAILURE);
		}
		pipe_report(&pipe_res);
		pipe_cleanup(&pipe_res);
		break;
	case 'm':
		// Shmem stuff
		if (shmem_init(argc, argv, &shmem_res) == false) {
			exit(EXIT_FAILURE);
		}
		break;
	case 'c':
		// Clean shmem
		shmem_cleanup();
		break;
	case 's':
		// Socket stuff
		if (sock_init(argc, argv, &sock_res) == false) {
			exit(EXIT_FAILURE);
		}
		sock_report(&sock_res);
		sock_cleanup(&sock_res);
		break;
	default:
		usage();
		break;
	}

	exit(EXIT_SUCCESS);
}

bool pipe_init(int argc, char **argv, struct pipe_res *res) {
	assert(res != NULL);

	if (argc < PIPE_ARGC) {
		usage();
	}

	res->nperfnums = 0;
	res->limit = atoi(argv[2]);
	res->nprocs = atoi(argv[3]);

	if (spawn_computes(
			&res->compute_pids,
			res->compute_pipe,
			res->limit,
			res->nprocs) == -1) {
		return false;
	}

	if (mkfifo(FIFO_PATH, FIFO_MODE) == -1) {
		perror("Could not make FIFO");
		return false;
	}

	res->report_fifo = open(FIFO_PATH, O_WRONLY);
	if (res->report_fifo == -1) {
		perror("Could not open FIFO");
		return false;
	}

	return true;
}

void pipe_report(struct pipe_res *res) {
	union packet packet;
	int bytes_read;
	int body_count = 0;

	assert(res != NULL);

	// Loop until signaled to quit
	while (body_count < res->nprocs) {
		bytes_read = get_packet(res->compute_pipe[READ], &packet);
		if (bytes_read == 0) {
			//break;
		} else if (bytes_read == -1) {
			if (errno != EAGAIN) {
				perror(NULL);
			}
		}

		if (bytes_read > 0) {
			switch (packet.id) {
			case PACKETID_PERFNUM:
				res->perfnums[res->nperfnums++] = packet.perfnum.perfnum;
				send_packet(res->report_fifo, &packet);
				break;
			case PACKETID_DONE:
				fprintf(stderr, "[manage] Received done\n");
				if (waitpid(packet.done.pid, NULL, 0) == -1) {
					perror(NULL);
				} else {
					body_count++;
				}
				// TODO: Give it another range
				break;
			case PACKETID_NULL:
			case PACKETID_RANGE:
				fprintf(stderr, "[manage] Invalid packet: %#02x\n", packet.id);
				break;
			default:
				fprintf(stderr, "[manage] Unrecognized packet: %#02x\n", packet.id);
				break;
			}
		}

		fflush(stderr);
	}
}

void pipe_cleanup(struct pipe_res *res) {
	union packet packet;

	assert(res != NULL);

	// Inform report that computation is finished
	packet.id = PACKETID_DONE;
	packet.done.pid = getpid();
	send_packet(res->report_fifo, &packet);

	// Clean up pipes
	close(res->compute_pipe[READ]);
	close(res->report_fifo);
	unlink(FIFO_PATH);

	free(res->compute_pids);
}

bool shmem_init(int argc, char **argv, struct shmem_res *res) {
	int total_size;
	int bitmap_size;
	int perfnums_size;
	int processes_size;
	int limit;

	assert(res != NULL);

	if (argc < SHMEM_ARGC) {
		usage();
	}

	limit = atoi(argv[2]);

	bitmap_size = limit / 8 + 1;
	perfnums_size = NPERFNUMS * sizeof(int);
	processes_size = NPROCS * sizeof(struct process);
	total_size = sizeof(int) + (2 * sizeof(sem_t)) + bitmap_size + perfnums_size + processes_size;

	if (shm_unlink(SHMEM_PATH) == -1) {
		if (errno != ENOENT) {
			perror("Could not unlink shared memory object");
			return false;
		}
	}

	res->addr = shmem_mount(SHMEM_PATH, total_size);
	res->limit = res->addr;
	res->bitmap_sem = res->limit + 1;
	res->bitmap = res->bitmap_sem + 1; // limit is a single integer, so bitmap is one int past
	res->perfect_numbers_sem = res->bitmap + bitmap_size;
	res->perfect_numbers = res->perfect_numbers_sem + 1;
	res->processes = res->perfect_numbers + NPERFNUMS;

	*res->limit = limit; // Set the limit in shared memory so other processes know

	if (sem_init(res->bitmap_sem, 1, 1) == -1) {
		perror("Could not initialize semaphore");
		return false;
	}

	if (sem_init(res->perfect_numbers_sem, 1, 1) == -1) {
		perror("Could not initialize semaphore");
		return false;
	}

	return true;
}

void shmem_cleanup(void) {
	struct shmem_res res;

	if (shmem_load(&res) == false) {
		return;

		bool all_tested(struct shmem_res *res);
	}

	while (sem_destroy(res.bitmap_sem) == -1) {
		if (errno == EINVAL) {
			break;
		}

		// Else something is currently blocking on the semaphore, keep up the attack
	}

	while (sem_destroy(res.perfect_numbers_sem) == -1) {
		if (errno == EINVAL) {
			break;
		}

		// Else something is currently blocking on the semaphore, keep up the attack
	}

	if (shm_unlink(SHMEM_PATH) == -1) {
		if (errno != ENOENT) {
			perror("Could not unlink shared memory object");
		}
	}
}

bool sock_init(int argc, char **argv, struct sock_res *res) {
	struct sockaddr_in servaddr;

	assert(res != NULL);

	if (argc < SOCK_ARGC) {
		usage();
	}

	res->listen = socket(AF_INET, SOCK_STREAM, 0);
	if (res->listen == -1) {
		perror("Could not create socket");
		return false;
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERVER_PORT);

	if (bind(res->listen, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
		perror("Unable to bind socket");
		return false;
	}

	if (listen(res->listen, MAX_BACKLOG) == -1) {
		perror("Unable to listen on socket");
	}

	res->maxfd = res->listen;
	res->maxi = -1;

	for (int i = 0; i < MAX_CLIENTS; i++) {
		res->clients[i] = -1; // Denotes an unused index
	}

	FD_ZERO(&res->allfds);
	FD_SET(res->listen, &res->allfds);

	return true;
}

void sock_report(struct sock_res *res) {
	struct sockaddr_in addr;
	socklen_t len;

	char buf[BUF_SIZE];
	int fd;
	int n;

	fd_set rset;
	int nready;

	while (1) {
		rset = res->allfds;
		nready = select(res->maxfd+1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(res->listen, &rset)) {
			// New client connection
			len = sizeof(addr);
			fd = accept(res->listen, (struct sockaddr *)&addr, &len);

			for (int i = 0; i <= FD_SETSIZE; i++) {
				if (i == MAX_CLIENTS) {
					perror("Client limit reached");
					close(fd); // Drop the client
				} else {
					if (res->clients[i] < 0) {
						if (i > res->maxi) {
							res->maxi = i;
						}

						res->clients[i] = fd;
						break;
					}
				}
			}

			FD_SET(fd, &res->allfds);
			if (fd > res->maxfd) {
				res->maxfd = fd;
			}

			if (--nready <= 0) {
				// No more readable descriptors
				continue;
			}
		}

		// Check all clients for data
		for (int i = 0; i <= res->maxi; i++) {
			fd = res->clients[i];
			if (fd < 0){
				continue;
			}

			if (FD_ISSET(fd, &rset)) {
				if ( (n = read(fd, buf, BUF_SIZE)) == 0) {
					// Connection closed by client
					close(fd);
					FD_CLR(fd, &res->allfds);
					res->clients[i] = -1;
				} else{
					write(fd, buf, n);
				}

				if (--nready <= 0) {
					// No more readable descriptors
					break;
				}
			}
		}
	}
}

void sock_cleanup(struct sock_res *res) {
}

int spawn_computes(pid_t **pids, int fds[2], int limit, int nprocs) {
	int flags;
	int numbers_per_proc = floor((double)limit / (double)nprocs);
	int end = 0;

	assert(pids != NULL);

	*pids = (pid_t *)malloc(limit * sizeof(pid_t));
	if (*pids == NULL) {
		perror("main");
		exit(EXIT_FAILURE);
	}

	if (pipe(fds) == -1) {
		perror("Unable to open compute pipe");
		return -1;
	}

	for (int i = 0; i < nprocs; i++) {
		pid_t pid;

		char start_str[11];
		char end_str[11];
		int start;

		// End is stored from previous loop
		start = end + 1;

		void shmem_report(struct shmem_res *res);
		// Weight the extra numbers to the front (started first, fastest check)
		if (i == 0)
		{
			end = numbers_per_proc + (limit % nprocs);
		} else {
			end = start + numbers_per_proc - 1;
		}

		snprintf(start_str, 11, "%d", start);
		snprintf(end_str, 11, "%d", end);

		pid = fork();
		if (pid > 0) {
			// Parent
			(*pids)[i] = pid;
		} else if (pid == 0) {
			// Child
			void shmem_report(struct shmem_res *res);

			// Duplicate write end of pipe to stdout
			if (dup2(fds[WRITE], STDOUT_FILENO) == -1) {
				perror("Could not duplicate file descriptor");
				return -1;
			}

			// Close read end of pipe
			close(fds[READ]);
			if (execl(COMPUTE_CMD, COMPUTE_CMD, "p", start_str, end_str, NULL) == -1) {
				perror("Unable to exec");
				return -1;
			}
		} else {
			// Error
			perror("Unable to spawn compute");
		}
	}

	// Now that all children have been spawned, close write end of pipe
	close(fds[WRITE]);

	if ((flags = fcntl(fds[READ], F_GETFL, 0)) == -1) {
		flags = 0;
	}
	
	if (fcntl(fds[READ], F_SETFL, flags | O_NONBLOCK) == -1) {
		perror("manage: fcntl");
		return -1;
	}

	return 0;
}

bool collect_zombies(struct pipe_res *res)
{
	pid_t pid = 0;
	static int body_count = 0;

	while ((pid > 0) && (body_count < res->nprocs)) {
		pid = waitpid(-1, NULL, WNOHANG);
		if (pid == 0) {
			// Nothing has exited
		} else if (pid == -1) {
			perror("manage");
		} else {
			// Zombie neutralized
			body_count++;
		}
	};

	return body_count == res->nprocs;
}

void *shmem_mount(char *path, int object_size) {
    int shmem_fd;
    void *addr;

    /* create and resize it */
    shmem_fd = shm_open(path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (shmem_fd == -1){
        perror("failed to open shared memory object");
        exit(EXIT_FAILURE);
    }
    /* resize it to something reasonable */
    if (ftruncate(shmem_fd, object_size) == -1){
        perror("failed to resize shared memory object");
        exit(EXIT_FAILURE);
    }

    addr = mmap(NULL, object_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmem_fd, 0);
    if (addr == MAP_FAILED){
        fprintf(stderr, "failed to map shared memory object\n");
        exit(EXIT_FAILURE);
    }

    return addr;
}

void usage(void) {
	fprintf(stdout, "Usage: manage [cmps] <limit> <nprocs>\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "Modes:\n");
	fprintf(stdout, "    c - clean up shared memory\n");
	fprintf(stdout, "        usage: manage c\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "    m - shared memory\n");
	fprintf(stdout, "        usage: manage m <limit>\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "        limit:      largest number to test\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "    p - pipes\n");
	fprintf(stdout, "        usage: manage p <limit> <nprocs>\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "        limit:      largest number to test\n");
	fprintf(stdout, "        nprocs:     number of compute processes to spawn\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "    s - sockets\n");
	fprintf(stdout, "        usage: manage s <sock args>\n");
	fprintf(stdout, "\n");

	exit(EXIT_FAILURE);
}

