/* Standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Local includes */
#include "socket_utils.h"

int
main(int argc, char ** argv)
{
  int ssock, csock, conn;
  struct sockaddr_in addr_info;
  socklen_t addr_len;
  char buffer[MAX_COMMAND_LENGTH];
  ssize_t s, r; size_t l;
  if (argc != 3) {
    fprintf(stderr, "USAGE: %s listen_port bank_port\n", argv[0]);
  }
  if ((ssock = init_server_socket(argv[1])) < 0) {
    fprintf(stderr, "ERROR: failed to start server\n");
    return EXIT_FAILURE;
  }
  if ((csock = init_client_socket(argv[2])) < 0) {
    fprintf(stderr, "ERROR: failed to connect to server\n");
    return EXIT_FAILURE;
  }
  while (1) {
    conn = accept(ssock, (struct sockaddr *)(&addr_info), &addr_len);
    r = recv(conn, buffer, MAX_COMMAND_LENGTH, 0);
    l = strnlen(buffer, MAX_COMMAND_LENGTH - 1) + 1;
    s = send(csock, buffer, l, 0);
    if (r != s) {
      fprintf(stderr, "%i bytes lost!\n", (int)(r - s));
    } else {
      buffer[MAX_COMMAND_LENGTH - 1] = '\0';
      printf("FW: %s\n", buffer);
    }
  }
  destroy_socket(csock);
  destroy_socket(ssock);
  return EXIT_SUCCESS;
}
