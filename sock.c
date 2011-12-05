/**
 * @file sock.c
 * @author Dan Albert
 * @date Created 11/30/2011
 * @date Last updated 12/05/2011
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * Defines functions for use with sockets implementation.
 *
 */
#include <arpa/inet.h>
#include <assert.h>
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

