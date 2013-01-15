/*
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Plouton. If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <errno.h>
#include <string.h>

// accept
#include <sys/types.h>
#include <sys/socket.h>
// inet_ntop
#include <arpa/inet.h>

#define CAST_ADDR(x) ((struct sockaddr *) &x)

int
handle_connection(int sock)
{
	struct sockaddr_in remote_addr;
	socklen_t remote_addr_len = sizeof(remote_addr);
	char addr_str[INET_ADDRSTRLEN];
	assert(sock >= 0);
	/* Attempt to connect using IPV6 */
	if ((conn = accept(sock, CAST_ADDR(remote_addr), &remote_addr_len)) >= 0) {
		fprintf(stderr, "INFO: tunnel established [%s:%hu]\n",
				inet_ntop(AF_INET, &remote_addr.sin_addr, addr_str, INET_ADDRSTRLEN),
				ntohs(remote_addr.sin_port));
	} else {
		fprintf(stderr, "ERROR: cannot accept connection (#%i: %s)\n",
				errno, strerror(errno));
	}
	return conn;
}
