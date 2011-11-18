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

  /* Acertain address information (including port number) */
  port_num = strtol(port, &residue, 10);
  #ifndef NDEBUG
  if (residue == NULL || *residue != '\0') {
    fprintf(stderr, "WARNING: ignoring extraneous characters\n");
  }
  printf("INFO: will attempt a connection on port %li\n", port_num);
  #endif
  if (port_num < MIN_PORT_NUM || port_num > MAX_PORT_NUM) {
    fprintf(stderr, "ERROR: port '%s' out of range [%i, %i]\n", port, MIN_PORT_NUM, MAX_PORT_NUM);
    return -1;
  }
  local_addr->sin_family = AF_INET;
  local_addr->sin_addr.s_addr = inet_addr(LOCAL_ADDRESS);
  local_addr->sin_port = htonl(port_num);

  /* Perform actual opening of the socket */
  // TODO a good non-zero value for protocol?
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
    return -1;
  }

  /* Perform connect */
  if (connect(sock, (struct sockaddr *)(&local_addr), addr_len)) {
    fprintf(stderr, "ERROR: unable to connect to socket\n");
    destroy_socket(sock);
    return -1;
  }
  #ifndef NDEBUG
  printf("INFO: connected to port %u\n", ntohl(local_addr.sin_port));
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
    return -1;
  }

  /* Perform bind and listen */
  if (bind(sock, (struct sockaddr *)(&local_addr), addr_len)) {
    fprintf(stderr, "ERROR: unable to bind socket\n");
    destroy_socket(sock);
    return -1;
  }
  if (listen(sock, MAX_CONNECTIONS)) {
    fprintf(stderr, "ERROR: unable to listen on socket\n");
    destroy_socket(sock);
    return -1;
  }
  #ifndef NDEBUG
  printf("INFO: listening on port %u\n", ntohl(local_addr.sin_port));
  #endif

  return sock;
}

#endif
