/**
 * @file packets.c
 * @author Dan Albert
 * @date Created 11/30/2011
 * @date Last updated 11/30/2011
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
 * Defines packet types for use with pipes and sockets.
 *
 */
#ifndef PACKETS_H
#define PACKETS_H

#include <unistd.h>

enum packet_id {
	PACKETID_NULL,
	PACKETID_DONE,
	PACKETID_RANGE,
	PACKETID_PERFNUM
};

struct packet_done {
	enum packet_id packet_id;
	pid_t pid;
};

struct packet_range {
	enum packet_id packet_id;
	int begin;
	int end;
};

struct packet_perfnum {
	enum packet_id packet_id;
	int perfnum;
};

union packet {
	enum packet_id id;
	struct packet_done done;
	struct packet_range range;
	struct packet_perfnum perfnum;
};

int get_packet(int fd, union packet *p);
void send_packet(int fd, union packet *p);

#endif // PACKETS_H