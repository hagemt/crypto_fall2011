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

#ifndef BANKING_RELAY_H
#define BANKING_RELAY_H

#include <time.h>

struct proxy_session_t {
	int csock, ssock, conn, count;
	enum mode_t { A2B, B2A } mode;
	time_t established, terminated;
	unsigned char buffer[BANKING_MAX_COMMAND_LENGTH];
};

#include <stddef.h>

/*! \brief 
 *
 * Returns 
 */
int handle_relay(ssize_t *, ssize_t *);

/*! \brief Wrap whatever accepts connections from a given socket.
 *
 * Returns a socket for the connection in question (might be invalid)
 */
int handle_connection(int);

#endif /* BANKING_RELAY_H */
