/* Standard includes */
#include <stdio.h>
#include <stdlib.h>

/* Readline includes */
#include <readline/readline.h>

/* SQLite includes */
#include <sqlite3.h>

/* Thread includes */
#include <pthread.h>

/* Local includes */
#include "banking_constants.h"
#define USE_BALANCE
#define USE_DEPOSIT
#include "banking_commands.h"
#include "socket_utils.h"

int
balance_command(const char * cmd)
{
  printf("Recieved command: %s\n", cmd);
  return 0;
}

int
deposit_command(const char * cmd)
{
  printf("Recieved command: %s\n", cmd);
  return 0;
}

struct account_t {
  char * name;
  double balance;
};

void
destroy_db(const char * db_path, sqlite3 * db_conn) {
  if (db_path && remove(db_path)) {
    fprintf(stderr, "WARNING: unable to delete database\n");
  }
  if (sqlite3_close(db_conn) != SQLITE_OK) {
    fprintf(stderr, "ERROR: cannot close database\n");
  }
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

struct thread_data_t {
  pthread_t tid;
  int sock;
  sqlite3 * db;
  pthread_mutex_t * accept_mutex;
};

void * handle_client(void * arg) {
  int conn;
  struct sockaddr_in remote_addr;
  socklen_t addr_len;
  struct thread_data_t * data = (struct thread_data_t *)arg;
  fprintf(stderr, "[thread %lu] INFO: child started\n", data->tid);

  pthread_mutex_lock(data->accept_mutex);
  conn = accept(data->sock, (struct sockaddr *)(&remote_addr), &addr_len);
  pthread_mutex_unlock(data->accept_mutex);

  if (conn >= 0) {
    fprintf(stderr, "[thread %lu] INFO: client connected\n", data->tid);
    close(conn);
  } else {
    fprintf(stderr, "[thread %lu] ERROR: failure to accept\n", data->tid);
  }

  fprintf(stderr, "[thread %lu] INFO: child stopped\n", data->tid);
  return NULL;
}

int
main(int argc, char ** argv)
{
  char * in;
  command cmd;
  int i;
  int ssock;
  sqlite3 * db_conn;
  int caught_signal;
  pthread_mutex_t accept_mutex;
  struct thread_data_t thread_data[MAX_CONNECTIONS];

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

  /* Thread initialization */
  pthread_mutex_init(&accept_mutex, NULL);
  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    thread_data[i].sock = ssock;
    thread_data[i].db = db_conn;
    thread_data[i].accept_mutex = &accept_mutex;
    if (pthread_create(&thread_data[i].tid, NULL, &handle_client, &thread_data[i])) {
      fprintf(stderr, "WARNING: failed to start worker thread\n");
    } else {
      thread_data[i].tid = (pthread_t)(BANKING_ERROR);
    }
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

  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    if (thread_data[i].tid != (pthread_t)(BANKING_ERROR)) {
      if (pthread_kill(thread_data[i].tid, 9)) {
        fprintf(stderr, "ERROR: failed to collect worker thread %d\n", i);
      } else {
        thread_data[i].tid = (pthread_t)(BANKING_OK);
      }
    }
  }
  pthread_mutex_destroy(&accept_mutex);

  destroy_socket(ssock);
  destroy_db(NULL, db_conn);

  printf("\n");
  return EXIT_SUCCESS;
}
