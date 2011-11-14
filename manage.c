/**
 * @file manage.c
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
 * Maintains the results of compute. It also keeps track of the active compute processes,
 * so that it can signal them to terminate.
 *
 */
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

/// Path to compute program
#define COMPUTE_CMD "./compute"

/// Path to compute program
#define REPORT_CMD "./report"

/// Number of child computer processes to spawn
#define NPROCS 8

/// Number to start testing values at (NOT FOR FINAL)
#define START_AT_STR "1"

#define READ 0
#define WRITE 1

int main(int argc, char **argv) {
	pid_t computes[NPROCS];
	pid_t report;
	int compute_pipe[2];
	int report_pipe[2];
	int body_count = 0;
	int flags;

	if (pipe(compute_pipe) == -1) {
		perror("Unable to open compute pipe");
		exit(1);
	}

	if (pipe(report_pipe) == -1) {
		perror("Unable to open report pipe");
		exit(1);
	}

	for (int i = 0; i < NPROCS; i++) {
		pid_t pid = fork();
		if (pid > 0) {
			// Parent

			computes[i] = pid;
		} else if (pid == 0) {
			// Child

			// Duplicate write end of pipe to stdout
			if (dup2(compute_pipe[WRITE], STDOUT_FILENO) == -1) {
				perror("Could not duplicate file descriptor");
				exit(1);
			}

			// Close read end of pipe
			close(compute_pipe[READ]);
			if (execl(COMPUTE_CMD, COMPUTE_CMD, START_AT_STR, NULL) == -1) {
				perror("Unable to exec");
				exit(1);
			}
		} else {
			// Error
			perror("Unable to spawn compute");
		}
	}

	report = fork();
	if (report > 0) {
		close(report_pipe[READ]);
	} else if (report == 0) {
		if (dup2(report_pipe[READ], STDIN_FILENO) == -1) {
			perror("Could not duplicate file descriptor");
			exit(1);
		}

		close(report_pipe[WRITE]);

		if (execl(REPORT_CMD, REPORT_CMD, NULL) == -1) {
			perror("Unable to exec report");
			exit(1);
		}
	} else {
		perror("Unable to fork report");
	}

	if (flags = fcntl(compute_pipe[READ], F_GETFL, 0) == -1) {
		flags = 0;
	}
	
	fcntl(compute_pipe[READ], F_SETFL, flags | O_NONBLOCK);

	while (body_count < NPROCS) {
		pid_t pid;
		int chars_read;
		char buf[PIPE_BUF];

		do {
			pid = waitpid(-1, NULL, WNOHANG);
			if (pid == report) {
				// report exited unexpectedly
				printf("report exited unexcpectedly\n");
			} else if (pid == 0) {
				// Nothing has exited
			} else if (pid == -1) {
				perror(NULL);
			} else {
				// Zombie neutralized
				body_count++;
			}
		} while ((pid > 0) && (body_count < NPROCS));

		chars_read = read(compute_pipe[READ], buf, PIPE_BUF);
		if (chars_read == 0) {
			printf("EOF\n");
			break;
		} else if (chars_read == -1) {
			if (errno != EAGAIN) {
				perror("FUBAR");
			}
		}
		
		if (chars_read > 0) {
			write(report_pipe[WRITE], buf, chars_read);
		}
	}

	// Now that all children have been spawned, close write end of pipe
	close(compute_pipe[WRITE]);

	// Close read end of pipe
	close(compute_pipe[READ]);

	close(report_pipe[WRITE]);

	waitpid(report, NULL, 0);

	while (body_count < NPROCS) {
		if (wait(NULL) == -1) {
			perror("Wait error");
		} else {
			body_count++;
		}
	}

	return 0;
}
