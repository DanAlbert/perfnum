/**
 * @file sock.c
 * @author Dan Albert
 * @date Created 11/30/2011
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
 * Defines functions for use with sockets implementation.
 *
 */
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include "sock.h"

int sock_connect(char *host) {
	struct sockaddr_in addr;
	int fd;

	assert(host != NULL);

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		perror("Unable to create socket");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	inet_pton(AF_INET, host, &addr.sin_addr);
	addr.sin_port = htons(SERVER_PORT);

	if (connect(fd, (struct sockaddr *)(&addr), sizeof(addr)) == -1) {
		perror("Unable to connect to server");
		return -1;
	}

	return fd;
}

