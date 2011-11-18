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

void
login_command(char * cmd)
{
  return;
}

void
balance_command(char * cmd)
{
  return;
}


void
withdraw_command(char * cmd)
{
  return;
}

void
logout_command(char * cmd)
{
  return;
}

void
transfer_command(char * cmd)
{
  return;
}

int
main(int argc, char ** argv)
{
  char * cmd;

  /* Input sanitation */
  if (argc != 2) {
    fprintf(stderr, "USAGE: %s port_num\n", argv[0]);
    return EXIT_FAILURE;
  }

  /* Issue an interactive prompt */
  while ((cmd = readline(PROMPT))) {
    if (validate(cmd)) {
      fprintf(stderr, "ERROR: invalid command: '%s'\n", cmd);
    } else {
      add_history(cmd);
      invoke(cmd);
    }
    free(cmd);
    cmd = NULL;
  }

  /* Cleanup */
  printf("\n");
  return EXIT_SUCCESS;
}
