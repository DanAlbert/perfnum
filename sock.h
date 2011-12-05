/**
 * @file sock.h
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

