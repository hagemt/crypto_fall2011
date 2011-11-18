#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "banking_constants.h"

#define USE_LOGIN
#define USE_BALANCE
#define USE_WITHDRAW
#define USE_LOGOUT
#define USE_TRANSFER
#include "banking_commands.h"

int
login_command(char * cmd)
{
  return 0;
}

int
balance_command(char * cmd)
{
  return 0;
}

int
withdraw_command(char * cmd)
{
  return 0;
}

int
logout_command(char * cmd)
{
  return 0;
}

int
transfer_command(char * cmd)
{
  return 0;
}

int
main(int argc, char ** argv)
{
  char * in;
  int caught_signal;
  command cmd;

  /* Input sanitation */
  if (argc != 2) {
    fprintf(stderr, "USAGE: %s port_num\n", argv[0]);
    return EXIT_FAILURE;
  }

  /* Issue an interactive prompt, terminate only on failure */
  for (caught_signal = 0; !caught_signal && (in = readline(PROMPT))) {
    /* Read in a line, then attempt to associate it with a command */
    if (validate(in, &cmd)) {
      fprintf(stderr, "ERROR: invalid command: '%s'\n", in);
    } else {
      /* Add valid commands to the shell history */
      add_history(in);
      /* Set up to signal based on the command's invocation */
      caught_signal = invoke(in, cmd);
    }
    free(in);
    in = NULL;
  }

  /* Cleanup */
  printf("\n");
  return EXIT_SUCCESS;
}
