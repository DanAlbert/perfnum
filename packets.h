/**
 * @file packets.c
 * @author Dan Albert
 * @date Created 11/30/2011
 * @date Last updated 12/05/2011
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * Defines packet types and declares functions for use with pipes and sockets.
 *
 */
#ifndef PACKETS_H
#define PACKETS_H

#include <unistd.h>

/// Server "pid" for closed packets in socket mode
#define PID_SERVER ((pid_t)0)

/// Client "pid" for closed packets in socket mode
#define PID_CLIENT ((pid_t)1)

/**
 * Packet identifier constants
 */
enum packet_id {
	PACKETID_NULL,
	PACKETID_DONE,
	PACKETID_CLOSED,
	PACKETID_KILL,
	PACKETID_RANGE,
	PACKETID_PERFNUM,
	PACKETID_NOTIFY,
	PACKETID_ACCEPT,
	PACKETID_REFUSE
};

/**
 * 'done' packet payload
 */
struct packet_done {
	enum packet_id packet_id;	///< Packet identifier
	pid_t pid;					///< Process ID of the sending process
};

/**
 * 'closed' packet payload
 */
struct packet_closed {
	enum packet_id packet_id;	///< Packet identifier
	pid_t pid;					///< Process ID of the sending process
};

/**
 * 'range' packet payload
 */
struct packet_range {
	enum packet_id packet_id;	///< Packet identifier
	int start;					///< Start of assigned range
	int end;					///< End of assigned range
};

/**
 * 'perfnum' packet payload
 */
struct packet_perfnum {
	enum packet_id packet_id;	///< Packet identifier
	int perfnum;				///< Perfect number
};

/**
 * General packet type. Ensures that sent packets always have the same size.
 */
union packet {
	enum packet_id id;
	struct packet_done done;
	struct packet_closed closed;
	struct packet_range range;
	struct packet_perfnum perfnum;
};

/**
 * @brief Read a packet from a stream, blocking until one is received
 *
 * Preconditions: fd is a valid file descriptor, p is not NULL
 *
 * Postconditions: Received packet has been loaded into memory pointed to by p
 *
 * @param fd File descriptor of the stream to read from
 * @param p Pointer to packet to load data into
 * @return -1 on error, 0 otherwise
 */
int get_packet(int fd, union packet *p);

/**
 * @brief Write a packet to a stream
 *
 * Preconditions: fd is a valid file descriptor, p is not NULL
 *
 * Postconditions: Packet has been sent over stream
 *
 * @param fd File descriptor of the stream to write to
 * @param p Pointer to packet to send
 * @return -1 on error, 0 otherwise
 */
int send_packet(int fd, union packet *p);

#endif // PACKETS_H

