/* Standard includes */
#include <stdio.h>
#include <stdlib.h>

/* Readline includes */
#include <readline/readline.h>

/* SQLite includes */
#include <sqlite3.h>

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

sqlite3 *
destroy_db(const char * db_path, sqlite3 * db_conn) {
  if (db_path && remove(db_path)) {
    fprintf(stderr, "WARNING: unable to delete database\n");
  }
  if (sqlite3_close(db_conn)) {
    fprintf(stderr, "ERROR: cannot close database\n");
  } else {
    db_conn = NULL;
  }
  return db_conn;
}

int
init_db(const char * db_path, sqlite3 * db_conn)
{
  if(sqlite3_open(db_path, &db_conn)) {
    fprintf(stderr, "ERROR: unable to open database\n");
    destroy_db(db_path, db_conn);
    return EXIT_FAILURE;
  }
  /* TODO add row for Adam, Bob, and Eve */
  return EXIT_SUCCESS;
}

int
main(int argc, char ** argv)
{
  char * in;
  command cmd;
  int ssock;
  sqlite3 * db_conn;
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

  /* Database initialization */
  if (init_db(":memory:", db_conn)) {
    fprintf(stderr, "FATAL: failed to connect to database\n");
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

  ssock = destroy_socket(ssock);
  db_conn = destroy_db(NULL, db_conn);
  printf("\n");
  return EXIT_SUCCESS;
}
