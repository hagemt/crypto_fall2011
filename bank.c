/* Standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Networking includes */
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

/* Readline includes */
#include <readline/readline.h>
#include <readline/history.h>

/* Local includes */
#define MAX_CONNECTIONS 5
#define USE_BALANCE
#define USE_DEPOSIT
#include "commands.h"

struct account_t {
  char * name;
  double balance;
};

int
main(int argc, char ** argv)
{
  int sock;
  char * cmd_buffer;
  long int port_number;
  struct addrinfo hints, * addr_info;
  socklen_t sock_len = sizeof(struct addrinfo);
  /* Sanitize input */
  if (argc != 2) {
    fprintf(stderr, "USAGE: %s port_number\n", argv[0]);
    return EXIT_FAILURE;
  }
  /* Read the port number as the singular argument */
  port_number = strtol(argv[1], &cmd_buffer, 10);
  #ifndef NDEBUG
  if (cmd_buffer == NULL || *cmd_buffer != '\0') {
    fprintf("WARNING: ignoring extraneous '%s'\n", cmd_buffer);
  }
  printf("INFO: will attempt to listen on port %l\n", port_number)
  #endif
  if (port_number < 0x400 || port_number > 0xFFFF) {
    fprintf(stderr, "ERROR: port '%s' out of range [1024, 65535]\n", argv[1]);
    return EXIT_FAILURE;
  }
  /* Set up address information */
  memset(&hints, 0, sock_len);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(LOCAL_ADDRESS, NULL, &hints, &addr_info)) {
    fprintf(stderr, "ERROR: unable to acertain '%s'\n", LOCAL_ADDRESS);
    return EXIT_FAILURE;
  }
  addr_info->sin_port = htonl(port_number);
  /* Open socket, bind, and listen */
  // TODO intelligent protocol? (tcp)
  if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "ERROR: unable to open socket\n");
    return EXIT_FAILURE;
  }
  if (bind(sock, (struct sockaddr *)addr_info, sock_len)) {
    fprintf(stderr, "ERROR: unable to bind socket\n");
    return EXIT_FAILURE;
  }
  if (listen(sock, MAX_CONNECTIONS)) {
    fprintf(stderr, "ERROR: unable to listen on socket\n");
    return EXIT_FAILURE;
  }
  #ifndef NDEBUG
  printf("INFO: listening on port %l\n", port_number);
  #endif
  while ((cmd_buffer = readline(PROMPT))) {
    if (validate(cmd)) { /* Catch invalid commands */
      fprintf(stderr, "ERROR: invalid command ''", )
    } else {
      add_history(cmd);
      invoke(cmd);
    }
    free(cmd_buffer);
    cmd_buffer = NULL;
  }
  /* Cleanup */
  freeaddrinfo(addr_info);
  if (close(sock)) {
    fprintf(stderr, "WARN: unable to close socket");
  }
  return EXIT_SUCCESS;
}
