/**
 * @file shmem.h
 * @author Dan Albert
 * @date Created 11/23/2011
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
 * Defines structures, constants and functions for use with the shared memory method.
 *
 */
#ifndef SHMEM_H
#define SHMEM_H

#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

/// Macro to get the value of a specific bit
#define BIT(byte, bit) ((byte >> bit) & 1)

/// Macro to set a specific bit
#define SET_BIT(byte, bit) (byte |= (1 << bit))

/// Macro to clear a specific bit
#define CLR_BIT(byte, bit) (byte &= ~(1 << bit))

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
};

struct shmem_res {
	void *addr;
	int *limit;
	pid_t *manage;
	sem_t *bitmap_sem;
	uint8_t *bitmap;
	sem_t *perfect_numbers_sem;
	int *perfect_numbers;
	struct process *processes;
	void *end;
};

bool shmem_load(struct shmem_res *res);

#endif // SHMEM_H

