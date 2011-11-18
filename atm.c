#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "atm_cmd.h"

#define PROMPT "[atorm0] $ "

int
main(int argc, char ** argv)
{
  char * cmd = NULL;
  if (argc != 2) {
    fprintf(stderr, "USAGE: %s port_num\n", argv[0]);
    return EXIT_FAILURE;
  }
  while ((cmd = readline(PROMPT))) {
    if (validate(cmd)) {
      fprintf(stderr, "Invalid command: '%s'\n", cmd);
    } else {
      fprintf(stdout, "'%s'\n", cmd);
    }
    free(cmd);
    cmd = NULL;
  }
  return EXIT_SUCCESS;
}
