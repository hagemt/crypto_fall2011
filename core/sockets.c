/**
 * Copyright 2011 by Tor E. Hagemann <hagemt@rpi.edu>
 * This file is part of Plouton.
 *
 * Plouton is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Plouton is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Plouton.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "libplouton.h"

#include <stdio.h>
//#include <stdlib.h>
#include <unistd.h>

/*
#include <arpa/inet.h>
#include <netdb.h>
*/
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>


// FIXME: use `typedef struct sockaddr_storage address_t;'

static inline int
__create_socket(const char *port, struct sockaddr_in *local_addr)
{
	int sock;
	char *residue;
	long int port_num;

	assert(port && local_addr);
	/* Ascertain address information (including port number) */
	port_num = strtol(port, &residue, 10);

#ifndef NDEBUG
	if (residue == NULL || *residue != '\0') {
		fprintf(stderr, "WARNING: ignoring '%s' (argument residue)\n", residue);
	}
	printf("INFO: will attempt to use port %li\n", port_num);
	/* TODO look more closely at overflow
	 * fprintf(stderr, "sizeof(local_addr->sin_port) = %lu\n",
	 *                  sizeof(local_addr->sin_port));
	 * fprintf(stderr, "sizeof((unsigned short)port_num) = %lu\n",
	 *                  sizeof((unsigned short)port_num));
	 * fprintf(stderr, "sizeof(htons((unsigned short)port_num)) = %lu\n",
	 *                  sizeof(htons((unsigned short)port_num)));
	 */
#endif

	/* Perform range-check on the port number */
	if (port_num < BANKING_PORT_MIN || port_num > BANKING_PORT_MAX) {
		fprintf(stderr, "ERROR: port '%s' outside valid range [%i, %i]\n",
				port, BANKING_PORT_MIN, BANKING_PORT_MAX);
		return BANKING_FAILURE;
	}

	/* Perform actual opening of the socket */
	local_addr->sin_family = AF_INET;
	local_addr->sin_addr.s_addr = inet_addr(BANKING_ADDR_IPV4);
	local_addr->sin_port = htons((unsigned short) port_num);
	/* TODO a good non-zero value for protocol? */
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "ERROR: unable to open socket\n");
	}
	return sock;
}

inline void
destroy_socket(int sock)
{
	/* Attempt to stop communication in both directions */
	if (shutdown(sock, SHUT_RDWR)) {
		fprintf(stderr, "WARNING: unable to shutdown socket\n");
	}
	/* Release the associated resources */
	if (close(sock)) {
		fprintf(stderr, "WARNING: unable to close socket\n");
	}
}

int
client_socket(const char *port)
{
	int sock;
	struct sockaddr_in local_addr;
	socklen_t addr_len = sizeof(local_addr);
	/* Create the underlying socket */
	if ((sock = __create_socket(port, &local_addr)) < 0) {
		fprintf(stderr, "ERROR: failed to create socket\n");
		return BANKING_FAILURE;
	}

	/* Perform connect */
	if (connect(sock, (const struct sockaddr *)(&local_addr), addr_len)) {
		fprintf(stderr, "ERROR: unable to connect to socket\n");
		__destroy_socket(sock);
		return BANKING_FAILURE;
	}

#ifndef NDEBUG
	printf("INFO: connected to port %hu\n", ntohs(local_addr.sin_port));
	/* TODO look more closely at overflow
	 * fprintf(stderr, "sizeof(local_addr.sin_port) = %lu\n",
	 *                  sizeof(local_addr.sin_port));
	 * fprintf(stderr, "sizeof(ntohs(local_addr.sin_port)) = %lu\n",
	 *                  sizeof(ntohs(local_addr.sin_port)));
	 */
#endif

	return sock;
}

inline int
server_socket(const char *port)
{
	int sock;
	struct sockaddr_in local_addr;
	socklen_t addr_len = sizeof(local_addr);
	/* Create the underlying socket */
	if ((sock = __create_socket(port, &local_addr)) < 0) {
		fprintf(stderr, "ERROR: failed to create socket\n");
		return BANKING_FAILURE;
	}

	/* Perform bind and listen */
	if (bind(sock, (const struct sockaddr *)(&local_addr), addr_len)) {
		fprintf(stderr, "ERROR: unable to bind socket\n");
		__destroy_socket(sock);
		return BANKING_FAILURE;
	}
	if (listen(sock, BANKING_MAX_CONNECTIONS)) {
		fprintf(stderr, "ERROR: unable to listen on socket\n");
		destroy_socket(sock);
		return BANKING_FAILURE;
	}

#ifndef NDEBUG
	printf("INFO: listening on port %hu\n", ntohs(local_addr.sin_port));
	/* TODO look more closely at (possible?) overflow
	 * fprintf(stderr, "sizeof(local_addr.sin_port) = %lu\n",
	 *                  sizeof(local_addr.sin_port));
	 * fprintf(stderr, "sizeof(ntohs(local_addr.sin_port)) = %lu\n",
	 *                  sizeof(ntohs(local_addr.sin_port)));
	 */
#endif

	return sock;
}

