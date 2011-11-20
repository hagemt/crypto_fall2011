/* Standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Networking includes */
/*#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>*/

/* Readline includes */
#include <readline/readline.h>

/* Local includes */
#include "banking_constants.h"
#define USE_BALANCE
#define USE_DEPOSIT
#include "banking_commands.h"
#include "socket_utils.h"

int
balance_command(char * cmd)
{
  printf("Recieved command: %s\n", cmd);
  return 0;
}

int
deposit_command(char * cmd)
{
  printf("Recieved command: %s\n", cmd);
  return 0;
}

struct account_t {
  char * name;
  double balance;
};

int
main(int argc, char ** argv)
{
  char * in;
  command cmd;
  int ssock;
  int caught_signal;

  /* Sanitize input and attempt socket initialization */
  if (argc != 2) {
    fprintf(stderr, "USAGE: %s port\n", argv[0]);
    return EXIT_FAILURE;
  }
  if ((ssock = init_server_socket(argv[1])) < 0) {
    fprintf(stderr, "FATAL: server failed to start\n");
    return EXIT_FAILURE;
  }

  /* Issue an interactive prompt, only quit on signal */
  for (caught_signal = 0; !caught_signal && (in = readline(PROMPT));) {
    /* Catch invalid commands */
    if (validate(in, &cmd)) {
      /* Ignore empty strings */
      if (*in != '\0') {
        fprintf(stderr, "ERROR: invalid command '%s'\n", in);
      }
    } else {
      /* Hook the command's return value to this signal */
      caught_signal = invoke(in, cmd);
    }
    /* Cleanup from here down */
    free(in);
    in = NULL;
  }

  destroy_socket(ssock);
  printf("\n");
  return EXIT_SUCCESS;
}
