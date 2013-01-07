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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "libplouton.h"

static struct proxy_session_data_t {
	/* A proxy connects clients to servers (intermediary layer) */
	int csock, ssock, conn, count;
	/* Its operation is modal, ATM --> BANK or BANK --> ATM */
	enum mode_t { A2B, B2A } mode;
	/* This is the relay buffer:*/
	unsigned char buffer[BANKING_MAX_COMMAND_LENGTH];
	/* Proxy tracks connection times */
	time_t established, terminated;
	/* Proxy handles signals */
	struct sigaction signal_action;
	sigset_t termination_signals;
} session_data;

void
handle_signal(int signum)
{
	if (signum) {
		fprintf(stderr,
				"WARNING: stopping proxy service [signal %i: %s]\n",
				signum, strsignal(signum));
	}

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
	memset(session_data.buffer, '\0', BANKING_MAX_COMMAND_LENGTH);

	/* Re-raise the proper termination signals */
	if (sigismember(&session_data.termination_signals, signum)) {
		sigemptyset(&session_data.signal_action.sa_mask);
		session_data.signal_action.sa_handler = SIG_DFL;
		sigaction(signum, &session_data.signal_action, NULL);
		raise(signum);
	}
}

#include <arpa/inet.h>

int
handle_connection(int sock, int *conn)
{
	char addr_str[INET_ADDRSTRLEN];
	struct sockaddr_in remote_addr;
	socklen_t remote_addr_len = sizeof(remote_addr);
	assert(sock >= 0 && conn);

	if ((*conn = accept(sock, (struct sockaddr *)(&remote_addr), &remote_addr_len)) >= 0) {
		/* Report successful connection information */
		fprintf(stderr, "INFO: tunnel established [%s:%hu]\n",
				inet_ntop(AF_INET, &remote_addr.sin_addr, addr_str, INET_ADDRSTRLEN),
				ntohs(remote_addr.sin_port));
		time(&session_data.established);
		return BANKING_SUCCESS;
	}
	return BANKING_FAILURE;
}

ssize_t
recv_relay(int sock)
{
	/* FIXME: Do logging? or maybe cryptanalysis */
	return recv(sock, session_data.buffer, BANKING_MAX_COMMAND_LENGTH, 0);
}

ssize_t
send_relay(int sock)
{
	/* FIXME: Do logging? or maybe cryptanalysis */
	return send(sock, session_data.buffer, BANKING_MAX_COMMAND_LENGTH, 0);
}

inline int
handle_relay(ssize_t *r, ssize_t *s) {
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
main(int argc, char ** argv)
{
	ssize_t received, sent, bytes;
	struct sigaction old_signal_action;

	/* Input sanitation */
	if (argc != 3) {
		fprintf(stderr, "USAGE: %s listen_port bank_port\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* Capture SIGINT and SIGTERM */
	memset(&session_data.signal_action, '\0', sizeof(struct sigaction));
	sigfillset(&session_data.signal_action.sa_mask);
	session_data.signal_action.sa_handler = &handle_signal;
	sigemptyset(&session_data.termination_signals);
	sigaction(SIGINT, NULL, &old_signal_action);
	if (old_signal_action.sa_handler != SIG_IGN) {
		sigaction(SIGINT, &session_data.signal_action, NULL);
		sigaddset(&session_data.termination_signals, SIGINT);
	}
	sigaction(SIGTERM, NULL, &old_signal_action);
	if (old_signal_action.sa_handler != SIG_IGN) {
		sigaction(SIGTERM, &session_data.signal_action, NULL);
		sigaddset(&session_data.termination_signals, SIGTERM);
	}

	/* Socket initialization */
	if ((session_data.csock = init_client_socket(argv[2])) < 0) {
		fprintf(stderr, "ERROR: unable to connect to server\n");
		return EXIT_FAILURE;
	}
	if ((session_data.ssock = init_server_socket(argv[1])) < 0) {
		fprintf(stderr, "ERROR: unable to start server\n");
		destroy_socket(session_data.csock);
		return EXIT_FAILURE;
	}

	/* Provide a dumb echo tunnel service TODO send/recv threads */
	while (!handle_connection(session_data.ssock, &session_data.conn)) {
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
		session_data.csock = init_client_socket(argv[2]);
	}

	/* Teardown, reuse signal handler */
	handle_signal(0);
	return EXIT_SUCCESS;
}

