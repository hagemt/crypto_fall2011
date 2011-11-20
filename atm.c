/* Standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Readline includes */
#include <readline/readline.h>

/* Local includes */
#include "banking_constants.h"
#define USE_LOGIN
#define USE_BALANCE
#define USE_WITHDRAW
#define USE_LOGOUT
#define USE_TRANSFER
#include "banking_commands.h"
#include "socket_utils.h"

int
login_command(char * cmd)
{
  printf("Recieved command: %s\n", cmd);
  return 0;
}

int
balance_command(char * cmd)
{
  printf("Recieved command: %s\n", cmd);
  return 0;
}

int
withdraw_command(char * cmd)
{
  printf("Recieved command: %s\n", cmd);
  return 0;
}

int
logout_command(char * cmd)
{
  printf("Recieved command: %s\n", cmd);
  return 0;
}

int
transfer_command(char * cmd)
{
  printf("Recieved command: %s\n", cmd);
  return 0;
}

int
main(int argc, char ** argv)
{
  char * in;
  command cmd;
  int csock, caught_signal;

  /* Input sanitation */
  if (argc != 2) {
    fprintf(stderr, "USAGE: %s port_num\n", argv[0]);
    return EXIT_FAILURE;
  }
  if ((csock = init_client_socket(argv[1])) < 0) {
    fprintf(stderr, "FATAL: proxy unreachable\n");
    return EXIT_FAILURE;
  }

  /* Issue an interactive prompt, terminate only on failure */
  for (caught_signal = 0; !caught_signal && (in = readline(PROMPT));) {
    /* Read in a line, then attempt to associate it with a command */
    if (validate(in, &cmd)) {
      /* Ignore empty commands */
      if (*in != '\0') {
        fprintf(stderr, "ERROR: invalid command '%s'\n", in);
      }
    } else {
      /* Set up to signal based on the command's invocation */
      caught_signal = invoke(in, cmd);
    }
    /* Cleanup from here down */
    free(in);
    in = NULL;
  }

  destroy_socket(csock);
  printf("\n");
  return EXIT_SUCCESS;
}
