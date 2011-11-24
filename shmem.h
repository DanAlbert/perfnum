/**
 * @file shmem.h
 * @author Dan Albert
 * @date Created 11/23/2011
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
 * Defines structures, constants and functions for use with the shared memory method.
 *
 */
#ifndef SHMEM_H
#define SHMEM_H

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

/// Name of shared memory object
#define SHMEM_PATH "albertd"

/// Maximum number of perfect numbers to store in shared memory
#define NPERFNUMS 20

/// Maximum number of processes to track in shared memory
#define NPROCS 20

struct process {
	pid_t pid;
	int found;
	int tested;
	int untested;
};

struct shmem_res {
	void *addr;
	int *limit;
	uint8_t *bitmap;
	int *perfect_numbers;
	struct process *processes;
};

bool shmem_load(struct shmem_res *res);

#endif // SHMEM_H
