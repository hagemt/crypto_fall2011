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

#define USE_LOGIN
#define USE_BALANCE
#define USE_WITHDRAW
#define USE_LOGOUT
#define USE_TRANSFER
#include "libplouton/client.h"

#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
	/* Input sanitation */
	if (argc != 2) {
		fprintf(stderr, "USAGE: %s port_num\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* Crypto initialization */
	if (init_crypto(old_shmid(&session_data.shmid))) {
		fprintf(stderr, "FATAL: unable to enter secure mode\n");
		return EXIT_FAILURE;
	}

	/* Socket initialization */
	if ((session_data.sock = client_socket(argv[1])) < 0) {
		fprintf(stderr, "FATAL: unable to connect to server\n");
		shutdown_crypto(old_shmid(&session_data.shmid));
		return EXIT_FAILURE;
	}

	/* Session initialization */
	do_handshake(&session_data);
	init_signals(&session_data);
	do_prompt(&session_data);

	/* Teardown */
	handle_signal(0);
	return EXIT_SUCCESS;
}

