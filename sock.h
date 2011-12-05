/**
 * @file sock.h
 * @author Dan Albert
 * @date Created 11/30/2011
 * @date Last updated 12/05/2011
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * Declares functions for use with sockets implementation.
 *
 */
#ifndef SOCK_H
#define SOCK_H

/// Port the server will listen on
#define SERVER_PORT 10054

/**
 * @brief Connect to the managing server
 *
 * Preconditions: host is not NULL, host is a valid address
 *
 * Postconditions: A connection has been made or an error has been reported
 *
 * @param host Address of the host on which the managing process is running
 * @return File descriptor of socket or -1 on error
 */
int sock_connect(char *host);

#endif // SOCK_H

