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

/* Local includes */
#include "socket_utils.h"

struct proxy_session_data_t {
  int csock, ssock, conn;
  unsigned char buffer[MAX_COMMAND_LENGTH];
} session_data;

void
handle_signal(int sig)
{
  #ifndef NDEBUG
  fprintf(stderr, "INFO: stopping proxy service [signal %i]\n", sig);
  #endif

  if (session_data.conn >= 0) {
    destroy_socket(session_data.conn);
  }
  if (session_data.ssock >= 0) {
    destroy_socket(session_data.ssock);
  }
  if (session_data.csock >= 0) {
    destroy_socket(session_data.csock);
  }

  /* TODO needs to flush buffer? */
  memset(session_data.buffer, 0, MAX_COMMAND_LENGTH);

  if (sig == SIGINT) {
    signal(SIGINT, SIG_DFL);
    kill(getpid(), SIGINT);
  }
}

inline int
handle_connection(int sock, int * conn)
{
  struct sockaddr_in addr_info;
  socklen_t addr_len = sizeof(addr_info);
  assert(sock >= 0 && conn);
  if ((*conn = accept(sock, (struct sockaddr *)&addr_info, &addr_len)) >= 0) {
    #ifndef NDEBUG
    fprintf(stderr, "INFO: tunnel established [%s:%hu]\n",
      inet_ntoa(addr_info.sin_addr), ntohs(addr_info.sin_port));
    #endif
    return BANKING_SUCCESS;
  }
  return BANKING_FAILURE;
}

inline int
handle_relay(struct proxy_session_data_t * session, ssize_t * r, ssize_t * s) {
  assert(session && session->buffer && r && s);
  memset(session->buffer, 0, MAX_COMMAND_LENGTH);
  if ((*r = recv(session->conn,  session->buffer, MAX_COMMAND_LENGTH, 0)) > 0
   && (*s = send(session->csock, session->buffer, MAX_COMMAND_LENGTH, 0)) > 0) {
    return BANKING_SUCCESS;
  }
  return BANKING_FAILURE;
}

int
main(int argc, char ** argv)
{
  ssize_t received, sent;

  if (argc != 3) {
    fprintf(stderr, "USAGE: %s listen_port bank_port\n", argv[0]);
    return EXIT_FAILURE;
  }
  signal(SIGINT, &handle_signal);
  signal(SIGTERM, &handle_signal);

  if ((session_data.csock = init_client_socket(argv[2])) < 0) {
    fprintf(stderr, "ERROR: unable to connect to server\n");
    return EXIT_FAILURE;
  }

  if ((session_data.ssock = init_server_socket(argv[1])) < 0) {
    fprintf(stderr, "ERROR: unable to start server\n");
    destroy_socket(session_data.csock);
    return EXIT_FAILURE;
  }

  while (!handle_connection(session_data.ssock, &session_data.conn)) {
    while (!handle_relay(&session_data, &received, &sent)) {
      if (sent != received) {
        fprintf(stderr, "%li bytes lost!\n", (long)(sent - received));
      }
      #ifndef NDEBUG
      session_data.buffer[MAX_COMMAND_LENGTH - 1] = '\0';
      printf("FW: '%s'\n", session_data.buffer);
      #endif
    }
    destroy_socket(session_data.conn);
  }

  handle_signal(0);
  return EXIT_SUCCESS;
}
