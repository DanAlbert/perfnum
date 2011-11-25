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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shmem.h"

void shmem_report(struct shmem_res *res);

void pipe_report(void);

int next_test(struct shmem_res *res);

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
		if (shmem_load(&res) == false) {
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
	case 'k':
		// Signal manage to shutdown computation
		printf("Kill not implemented\n");
		exit(EXIT_FAILURE);
	default:
		printf("Invalid mode\n");
		break;
	}

	exit(EXIT_SUCCESS);
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

void pipe_report(void) {
	char c = getchar();
	while (c != EOF) {
		putchar(c);
		c = getchar();
	}
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

