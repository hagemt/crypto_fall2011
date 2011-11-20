/* Standard includes */
#include <stdio.h>
#include <stdlib.h>

/* Readline includes */
#include <readline/readline.h>

/* Thread includes */
#include <pthread.h>
#include <signal.h>

/* Local includes */
#include "banking_constants.h"

#define USE_BALANCE
#define USE_DEPOSIT
#include "banking_commands.h"

#include "socket_utils.h"

#include "db_utils.h"

struct server_session_data_t {
  int sock, caught_signal;
  sqlite3 * db_conn;
  pthread_mutex_t accept_mutex;
  pthread_t tids[MAX_CONNECTIONS];
} session_data;

int
balance_command(const char * args)
{
  sqlite3_stmt * statement;
  const char * residue;
  char * query = malloc(2 * MAX_COMMAND_LENGTH);
  snprintf(query, MAX_COMMAND_LENGTH, "SELECT balance FROM accounts WHERE name='%s';", args);
  if (sqlite3_prepare(session_data.db_conn, query, MAX_COMMAND_LENGTH, &statement, &residue) == SQLITE_OK) {
    while (sqlite3_step(statement) != SQLITE_DONE) {
      residue = (const char *)sqlite3_column_text(statement, 0);
      printf("%s's balance is $%i\n", residue, sqlite3_column_int(statement, 1));
    }
  }
  sqlite3_finalize(statement);
  free(query);
  return BANKING_OK;
}

int
deposit_command(const char * cmd)
{
  /* TODO write proper query code */
  printf("'%s'\n", cmd);
  return BANKING_OK;
}

void
handle_signal()
{
  int i;
  session_data.caught_signal = 1;
  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    if (session_data.tids[i] != (pthread_t)(BANKING_ERROR)) {
      fprintf(stderr, "INFO: sending SIGTERM to worker thread\n");
      pthread_kill(session_data.tids[i], SIGTERM);
    }
  }
  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    if (session_data.tids[i] != (pthread_t)(BANKING_ERROR)) {
      if (pthread_join(session_data.tids[i], NULL)) {
        fprintf(stderr, "ERROR: failed to collect worker thread\n");
      } else {
        fprintf(stderr, "INFO: collected worker thread\n");
        session_data.tids[i] = (pthread_t)(BANKING_OK);
      }
    }
  }
  pthread_mutex_destroy(&session_data.accept_mutex);
  destroy_socket(session_data.sock);
  destroy_db(NULL, session_data.db_conn);
}

inline void
retire(int sig) {
  fprintf(stderr, "INFO: worker thread retiring [signal %i]\n", sig);
  pthread_exit(NULL);
}

void *
handle_client(void * arg)
{
  int conn;
  char buffer[MAX_COMMAND_LENGTH];
  struct sockaddr_in remote_addr;
  socklen_t addr_len;
  pthread_t * tid;

  /* Worker threads should terminate on SIGTERM */
  signal(SIGTERM, retire);

  /* Fetch the ID from the argument */
  tid = (pthread_t *)arg;
  fprintf(stderr, "[thread %lu] INFO: worker started\n", *tid);

  /* As long as possible, grab up whatever connection is available */
  while (!session_data.caught_signal) {
    pthread_mutex_lock(&session_data.accept_mutex);
    conn = accept(session_data.sock, (struct sockaddr *)(&remote_addr), &addr_len);
    pthread_mutex_unlock(&session_data.accept_mutex);
    if (conn >= 0) {
      fprintf(stderr, "[thread %lu] INFO: worker connected to client\n", *tid);
      recv(conn, buffer, MAX_COMMAND_LENGTH, 0);
      #ifndef NDEBUG
      buffer[MAX_COMMAND_LENGTH - 1] = '\0';
      fprintf(stderr, "[thread %lu] RECV: '%s'\n", *tid, buffer);
      #endif
      destroy_socket(conn);
    } else {
      fprintf(stderr, "[thread %lu] ERROR: worker failed to accept client\n", *tid);
    }
  }

  retire(0);
  return NULL;
}

int
main(int argc, char ** argv)
{
  int i;
  char * in;
  command cmd;

  /* Sanitize input and attempt socket initialization */
  if (argc != 2) {
    fprintf(stderr, "USAGE: %s port\n", argv[0]);
    return EXIT_FAILURE;
  }
  if ((session_data.sock = init_server_socket(argv[1])) < 0) {
    fprintf(stderr, "FATAL: server failed to start\n");
    return EXIT_FAILURE;
  }

  /* Database initialization */
  if (init_db(":memory:", session_data.db_conn)) {
    fprintf(stderr, "FATAL: failed to connect to database\n");
    return EXIT_FAILURE;
  }

  /* Thread initialization */
  session_data.caught_signal = 0;
  pthread_mutex_init(&session_data.accept_mutex, NULL);
  signal(SIGINT, handle_signal);
  signal(SIGTERM, handle_signal);
  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    if (pthread_create(&session_data.tids[i], NULL, &handle_client, &session_data.tids[i])) {
      session_data.tids[i] = (pthread_t)(BANKING_ERROR);
      fprintf(stderr, "WARNING: failed to start worker thread\n");
    }
  }

  /* Issue an interactive prompt, only quit on signal */
  while (!session_data.caught_signal && (in = readline(PROMPT))) {
    /* Catch invalid commands */
    if (validate(in, &cmd)) {
      /* Ignore empty strings */
      if (*in != '\0') {
        fprintf(stderr, "ERROR: invalid command '%s'\n", in);
      }
    } else {
      /* Hook the command's return value to this signal */
      session_data.caught_signal = invoke(in, cmd);
    }
    /* Cleanup from here down */
    free(in);
    in = NULL;
  }
  printf("\n");
  handle_signal();
  return EXIT_SUCCESS;
}
