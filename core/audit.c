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

inline void *
strong_random_bytes(size_t len)
{
	return gcry_random_bytes_secure(len, GCRY_STRONG_RANDOM);
}

/* We wrap read/write on the socket for simple auditing */

ssize_t
send_message(const struct buffet_t *buffet, int sock)
{
	assert(buffet && sock >= 0);
	/* TODO AAA features? */
	return write(sock, buffet->cbuffer, BANKING_MAX_COMMAND_LENGTH);
	/* TODO assurance that only this function can write to sock? */
}

ssize_t
recv_message(const struct buffet_t *buffet, int sock)
{
	assert(buffet && sock >= 0);
	/* TODO AAA features? */
	return read(sock, buffet->cbuffer, BANKING_MAX_COMMAND_LENGTH);
	/* TODO assurance that only this function can write to sock? */
}
