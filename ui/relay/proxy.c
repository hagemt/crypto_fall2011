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
#include <stdlib.h>

extern struct proxy_session_t session;

inline void
clear_buffer()
{
	memset(session_data.buffer, '\0', BANKING_MAX_COMMAND_LENGTH);
}

void
close_session()
{
	/* Attempt to disconnect from everything */
	if (session_data.conn >= 0) {
		destroy_socket(session_data.conn);
	}
	if (session_data.ssock >= 0) {
		destroy_socket(session_data.ssock);
	}
	if (session_data.csock >= 0) {
		destroy_socket(session_data.csock);
	}

	/* Clear the internal buffer */
	clear_buffer();
}

int
handle_relay(ssize_t *r, ssize_t *s)
{
	assert(r && s);

	/* We will buffer a single message */
	memset(session_data.buffer, '\0', BANKING_MAX_COMMAND_LENGTH);

	/* Handle an ATM to BANK communication */
	if (session_data.mode == A2B) {
		if ((*r = recv_relay(session_data.conn)) > 0 &&
				(*s = send_relay(session_data.csock)) > 0) {
			++session_data.count;
			session_data.mode = B2A;
			return BANKING_SUCCESS;
		}
	}

	/* Handle a BANK to ATM communication */
	if (session_data.mode == B2A) {
		if ((*r = recv_relay(session_data.csock)) > 0 &&
				(*s = send_relay(session_data.conn)) > 0) {
			++session_data.count;
			session_data.mode = A2B;
			return BANKING_SUCCESS;
		}
	}

	return BANKING_FAILURE;
}

int
main(int argc, char *argv[])
{
	ssize_t received, sent, bytes;

	/* Input sanitation */
	if (argc != 3) {
		fprintf(stderr, "USAGE: %s listen_port bank_port\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* Socket initialization */
	if ((atm = client_socket(argv[2])) < 0) {
		fprintf(stderr, "ERROR: unable to connect to server\n");
		return EXIT_FAILURE;
	}
	if ((session_data.ssock = server_socket(argv[1])) < 0) {
		fprintf(stderr, "ERROR: unable to start server\n");
		destroy_socket(session_data.csock);
		return EXIT_FAILURE;
	}

	/* Provide a dumb echo tunnel service TODO send/recv threads */
	while (!handle_connection(session_data.ssock, &session_data.conn)) {
		time(&session_data.established);
		session_data.mode = A2B;
		session_data.count = 0;
		while (!handle_relay(&received, &sent)) {

			/* Report leaky transmissions */
			if (sent != received) {
				bytes = sent - received;
				if (bytes < 0) { bytes = -bytes; }
				fprintf(stderr, "ERROR: %li byte(s) lost\n", (long)(bytes));
			}

			/* NOTE: modality is swapped after relay */
			if (session_data.mode == A2B) {
				fprintf(stderr,
						"INFO: server sent message [id: %08i]\n",
						session_data.count);
			}
			if (session_data.mode == B2A) {
				fprintf(stderr,
						"INFO: client sent message [id: %08i]\n",
						session_data.count);
			}

#ifndef NDEBUG
			/* Report entire transmission */
			hexdump(stderr, session_data.buffer, BANKING_MAX_COMMAND_LENGTH);
#endif
		}

		/* Print statistics, following disconnection */
		time(&session_data.terminated);
		fprintf(stderr, "INFO: tunnel closed [%i msg | %li sec]\n",
				session_data.count,
				(long)(session_data.terminated - session_data.established));

		/* Disconnect from defunct clients */
		destroy_socket(session_data.conn);

		/* Re-establish with the server TODO should this be necessary? */
		destroy_socket(session_data.csock);
		session_data.csock = client_socket(argv[2]);
	}

	/* Teardown, reuse signal handler */
	handle_signal(0);
	return EXIT_SUCCESS;
}

