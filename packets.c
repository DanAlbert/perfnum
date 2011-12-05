/**
 * @file packets.c
 * @author Dan Albert
 * @date Created 11/30/2011
 * @date Last updated 12/05/2011
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * Defines packet types and functions for use with pipes and sockets.
 *
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "packets.h"

int get_packet(int fd, union packet *p) {
	assert(p != NULL);

	memset(p, 0, sizeof(union packet));
	return read(fd, p, sizeof(union packet));
}

int send_packet(int fd, union packet *p) {
	assert(p != NULL);

	return write(fd, p, sizeof(union packet));
}

