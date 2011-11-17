/**
 * @file compute.c
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
 * Computes perfect numbers. It tests all numbers beginning from its starting point,
 * subject to the constraints below. There may be more than one copy of compute running
 * simultaneously.
 *
 */
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

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

int main(int argc, char **argv) {
	unsigned int start;
	unsigned int end;
	
	if (argc < 3) {
		printf("Test limits not specified.\n");
		exit(1);
	}

	start = atoi(argv[1]);
	end = atoi(argv[2]);

	for (unsigned int i = start; i <= end; i++) {
		if (is_perfect_number(i) == true) {
			printf("%d\n", i);
		}
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

