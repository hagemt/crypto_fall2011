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

/* These functions are linux-specific */

#include <unistd.h>

/*! \brief Wrap whatever receieves raw bytes from a socket.
 *
 * Return the number of bytes received. See: man recv(3posix)
 */
inline ssize_t
recv_relay(int sock)
{
	ssize_t result;
	assert(sock >= 0);
	/* FIXME: Do logging? or maybe cryptanalysis */
	result = recv(sock, session_data.buffer, BANKING_MAX_COMMAND_LENGTH, 0);
	return result;
}

/*! \brief Wrap whatever sends raw bytes to a socket.
 *
 * Returns the number of bytes sent. See: man send(3posix)
 */
inline ssize_t
send_relay(int sock)
{
	ssize_t result;
	assert(sock >= 0);
	/* FIXME: Do logging? or maybe cryptanalysis */
	result = send(sock, session_data.buffer, BANKING_MAX_COMMAND_LENGTH, 0);
	return result;
}


