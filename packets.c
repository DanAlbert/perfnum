/**
 * @file packets.c
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
 * Defines packet types and functions for use with pipes and sockets.
 *
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "packets.h"

int get_packet(int fd, union packet *p) {
	memset(p, 0, sizeof(union packet));
	return read(fd, p, sizeof(union packet));
}

int send_packet(int fd, union packet *p) {
	return write(fd, p, sizeof(union packet));
}

