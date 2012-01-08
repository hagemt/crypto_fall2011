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

/* Standard includes */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

/* Local includes */
#include "socket_utils.h"

struct proxy_session_data_t {
  int csock, ssock, conn, count;
  time_t established, terminated;
  unsigned char buffer[MAX_COMMAND_LENGTH];
} session_data;

void
handle_signal(int signum)
{
  fprintf(stderr, "WARNING: stopping proxy service [signal %i: %s]\n", signum, strsignal(signum));

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
  memset(session_data.buffer, '\0', MAX_COMMAND_LENGTH);

  /* Re-raise the proper termination signals */
  if (signum == SIGINT || signum == SIGTERM) {
    signal(signum, SIG_DFL);
    raise(signum);
  }
}

int
handle_connection(int sock, int * conn)
{
  char addr_buffer[INET_ADDRSTRLEN];
  struct sockaddr_in remote_addr;
  socklen_t addr_len = sizeof(remote_addr);
  assert(sock >= 0 && conn);
  if ((*conn = accept(sock, (struct sockaddr *)(&remote_addr), &addr_len)) >= 0) {
    time(&session_data.established);
    /* Report successful connection information */
    fprintf(stderr, "INFO: tunnel established [%s:%hu]\n",
      inet_ntop(AF_INET, &remote_addr.sin_addr, addr_buffer, INET_ADDRSTRLEN),
      ntohs(remote_addr.sin_port));
    return BANKING_SUCCESS;
  }
  return BANKING_FAILURE;
}

inline int
handle_relay(struct proxy_session_data_t * session, ssize_t * r, ssize_t * s) {
  assert(session && session->buffer && r && s);
  /* Echo a single message */
  memset(session->buffer, '\0', MAX_COMMAND_LENGTH);
  if ((*r = recv(session->conn,  session->buffer, MAX_COMMAND_LENGTH, 0)) > 0
   && (*s = send(session->csock, session->buffer, MAX_COMMAND_LENGTH, 0)) > 0) {
    ++session_data.count;
    return BANKING_SUCCESS;
  }
  return BANKING_FAILURE;
}

int
main(int argc, char ** argv)
{
  ssize_t received, sent, bytes;

  /* Input sanitation */
  if (argc != 3) {
    fprintf(stderr, "USAGE: %s listen_port bank_port\n", argv[0]);
    return EXIT_FAILURE;
  }

  /* Capture SIGINT and SIGTERM, TODO sigaction? */
  if (signal(SIGINT, &handle_signal) == SIG_IGN) {
    signal(SIGINT, SIG_IGN);
  }
  if (signal(SIGTERM, &handle_signal) == SIG_IGN) {
    signal(SIGTERM, SIG_IGN);
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

  /* Provide a dumb echo tunnel service */
  while (!handle_connection(session_data.ssock, &session_data.conn)) {
    session_data.count = 0;
    while (!handle_relay(&session_data, &received, &sent)) {
      /* Report leaky transmissions */
      if (sent != received) {
        bytes = sent - received;
        if (bytes < 0) { bytes = -bytes; }
        fprintf(stderr, "ERROR: %li byte(s) lost\n", (long)(bytes));
      }
      #ifndef NDEBUG
      /* Report entire transmission */
      hexdump(session_data.buffer, MAX_COMMAND_LENGTH);
      #endif
    }
    time(&session_data.terminated);
    /* Disconnect from defunct clients */
    destroy_socket(session_data.conn);
    fprintf(stderr, "INFO: tunnel closed [%i sent in %li sec]\n",
      session_data.count, (long)(session_data.terminated - session_data.established));
  }

  /* Teardown */
  handle_signal(0);
  return EXIT_SUCCESS;
}
