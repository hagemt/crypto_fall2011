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

/* We wrap read/write on the socket for simple auditing */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

ssize_t
send_message(const struct buffet_t *buffet, int sock)
{
	ssize_t result;
	assert(buffet && sock >= 0);
	/* TODO AAA features? */
	errno = 0;
	result = write(sock, buffet->cbuffer, BANKING_MAX_COMMAND_LENGTH);
	/* TODO assurance that only this function can write to sock? */
	if (result != BANKING_MAX_COMMAND_LENGTH) {
		fprintf(stderr, "[WARNING] code %i (write failed on %i)\n", errno, sock);
#ifndef NDEBUG
		if (result != -1) {
			fprintf(stderr, "[INFO] %i bytes (where %i were expected)\n",
					(int) result, BANKING_MAX_COMMAND_LENGTH);
		} else {
			perror("[DEBUG] ");
		}
#endif /* DEBUG */
	}
	return result;
}

ssize_t
recv_message(const struct buffet_t *buffet, int sock)
{
	ssize_t result;
	assert(buffet && sock >= 0);
	/* TODO AAA features? */
	errno = 0;
	result = read(sock, (void *) buffet->cbuffer, BANKING_MAX_COMMAND_LENGTH);
	/* TODO assurance that only this function can write to sock? */
	if (result != BANKING_MAX_COMMAND_LENGTH) {
		fprintf(stderr, "[WARNING] code %i (read failed on %i)\n", errno, sock);
#ifndef NDEBUG
		if (result != -1) {
			fprintf(stderr, "[INFO] %i bytes (where %i were expected)\n",
					(int) result, BANKING_MAX_COMMAND_LENGTH);
		} else {
			perror("[DEBUG] ");
		}
#endif /* DEBUG */
	}
	return result;
}

inline void
show_message(struct buffet_t *buffet)
{
	buffet->tbuffer[BANKING_MAX_COMMAND_LENGTH - 1] = '\0';
	printf("%s\n", buffet->tbuffer);
	clear_buffet(buffet);
}

