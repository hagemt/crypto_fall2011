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
#include "banking_constants.h"
#define USE_BALANCE
#define USE_DEPOSIT
#include "banking_commands.h"

int
balance_command(char * cmd)
{
  return 0;
}

int
deposit_command(char * cmd)
{
  return 0;
}

struct account_t {
  char * name;
  double balance;
};

int
main(int argc, char ** argv)
{
  int ssock, csock, caught_signal;
  char * in;
  command cmd;
  long int port;
  struct sockaddr_in local_addr;
  socklen_t addr_len = sizeof(local_addr);
  /* Sanitize input */
  if (argc != 2) {
    fprintf(stderr, "USAGE: %s port\n", argv[0]);
    return EXIT_FAILURE;
  }

  /* Read the port number as the singular argument */
  port = strtol(argv[1], &in, 10);
  #ifndef NDEBUG
  if (in == NULL || *in != '\0') {
    fprintf(stderr, "WARNING: ignoring extraneous '%s'\n", in);
  }
  printf("INFO: will attempt to listen on port %li\n", port);
  #endif
  if (port < 0x400 || port > 0xFFFF) {
    fprintf(stderr, "ERROR: port '%s' out of range [1024, 65535]\n", argv[1]);
    return EXIT_FAILURE;
  }

  /* Set up address information, open socket, bind, and listen */
  local_addr.sin_family = AF_INET;
  local_addr.sin_addr.s_addr = inet_addr(LOCAL_ADDRESS);
  local_addr.sin_port = htonl(port);
  // TODO intelligent protocol? (tcp)
  if ((ssock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "ERROR: unable to open socket\n");
    return EXIT_FAILURE;
  }
  if (bind(ssock, (struct sockaddr *)(&local_addr), addr_len)) {
    fprintf(stderr, "ERROR: unable to bind socket\n");
    return EXIT_FAILURE;
  }
  if (listen(ssock, MAX_CONNECTIONS)) {
    fprintf(stderr, "ERROR: unable to listen on socket\n");
    return EXIT_FAILURE;
  }
  #ifndef NDEBUG
  printf("INFO: listening on port %li\n", port);
  #endif

  /* Issue an interactive prompt, only quit on signal */
  for (caught_signal = 0; !caught_signal && (in = readline(PROMPT))) {
    if (validate(in, &cmd)) {
      /* Catch invalid commands */
      fprintf(stderr, "ERROR: invalid command '%s'\n", in);
    } else {
      add_history(in);
      caught_signal = invoke(in, cmd);
    }
    free(in);
    in = NULL;
  }

  /* Cleanup */
  if (close(ssock)) {
    fprintf(stderr, "WARN: unable to close socket\n");
  }
  printf("\n");
  return EXIT_SUCCESS;
}
