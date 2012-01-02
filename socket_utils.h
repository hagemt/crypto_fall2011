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

#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

/* Standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Networking includes */
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

/* Local includes */
#include "banking_constants.h"

void
destroy_socket(int sock)
{
  /* Attempt to stop communication in both directions */
  if (shutdown(sock, 2)) {
    fprintf(stderr, "WARNING: unable to shutdown socket\n");
  }
  if (close(sock)) {
    fprintf(stderr, "WARNING: unable to close socket\n");
  }
}

int
create_socket(const char * port, struct sockaddr_in * local_addr)
{
  int sock;
  char * residue;
  long int port_num;

  /* Ascertain address information (including port number) */
  port_num = strtol(port, &residue, 10);
  #ifndef NDEBUG
  if (residue == NULL || *residue != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (argument residue)\n", residue);
  }
  printf("INFO: will attempt to use port %li\n", port_num);
  /* TODO look more closely at overflow
   * printf("sizeof(local_addr->sin_port) = %lu\n", sizeof(local_addr->sin_port));
   * printf("sizeof((unsigned short)port_num) = %lu\n", sizeof((unsigned short)port_num));
   * printf("sizeof(htons((unsigned short)port_num)) = %lu\n", sizeof(htons((unsigned short)port_num)));
   */
  #endif
  if (port_num < MIN_PORT_NUM || port_num > MAX_PORT_NUM) {
    fprintf(stderr, "ERROR: port '%s' out of range [%i, %i]\n", port, MIN_PORT_NUM, MAX_PORT_NUM);
    return BANKING_FAILURE;
  }
  local_addr->sin_family = AF_INET;
  local_addr->sin_addr.s_addr = inet_addr(BANKING_ADDRESS);
  local_addr->sin_port = htons((unsigned short)(port_num));

  /* Perform actual opening of the socket */
  /* TODO a good non-zero value for protocol? */
  if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "ERROR: unable to open socket\n");
  }
  return sock;
}

int
init_client_socket(const char * port)
{
  int sock;
  struct sockaddr_in local_addr;
  socklen_t addr_len = sizeof(local_addr);
  if ((sock = create_socket(port, &local_addr)) < 0) {
    fprintf(stderr, "ERROR: failed to create socket\n");
    return BANKING_FAILURE;
  }

  /* Perform connect */
  if (connect(sock, (struct sockaddr *)(&local_addr), addr_len)) {
    fprintf(stderr, "ERROR: unable to connect to socket\n");
    destroy_socket(sock);
    return BANKING_FAILURE;
  }
  #ifndef NDEBUG
  printf("INFO: connected to port %hu\n", ntohs(local_addr.sin_port));
  /* TODO look more closely at overflow
   * fprintf(stderr, "sizeof(local_addr.sin_port) = %lu\n", sizeof(local_addr.sin_port));
   * fprintf(stderr, "sizeof(ntohs(local_addr.sin_port)) = %lu\n", sizeof(ntohs(local_addr.sin_port)));
   */
  #endif

  return sock;
}

int
init_server_socket(const char * port)
{
  int sock;
  struct sockaddr_in local_addr;
  socklen_t addr_len = sizeof(local_addr);
  if ((sock = create_socket(port, &local_addr)) < 0) {
    fprintf(stderr, "ERROR: failed to create socket\n");
    return BANKING_FAILURE;
  }

  /* Perform bind and listen */
  if (bind(sock, (struct sockaddr *)(&local_addr), addr_len)) {
    fprintf(stderr, "ERROR: unable to bind socket\n");
    destroy_socket(sock);
    return BANKING_FAILURE;
  }
  if (listen(sock, MAX_CONNECTIONS)) {
    fprintf(stderr, "ERROR: unable to listen on socket\n");
    destroy_socket(sock);
    return BANKING_FAILURE;
  }
  #ifndef NDEBUG
  printf("INFO: listening on port %hu\n", ntohs(local_addr.sin_port));
  /* TODO look more closely at overflow
   * fprintf(stderr, "sizeof(local_addr.sin_port) = %lu\n", sizeof(local_addr.sin_port));
   * fprintf(stderr, "sizeof(ntohs(local_addr.sin_port)) = %lu\n", sizeof(ntohs(local_addr.sin_port)));
   */
  #endif

  return sock;
}

#endif
