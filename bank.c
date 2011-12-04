/**
 * Copyright 2011 by Tor E. Hagemann <hagemt@rpi.edu>
 * This file is part of Plouton.
 *
 * Plouton is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Plouton is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Plouton.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Standard includes */
#include <stdio.h>
#include <stdlib.h>

/* Readline includes */
#include <readline/readline.h>
#include <readline/history.h>

/* Thread includes */
#include <pthread.h>
#include <signal.h>

/* Local includes */
#define USE_BALANCE
#define USE_DEPOSIT
#include "banking_constants.h"
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
balance_command(char * args)
{
  size_t len, i;
  char * username;
  long int balance;
  const char * residue;

  /* Advance to the first token, this is the username */
  len = strnlen(args, MAX_COMMAND_LENGTH);
  for (i = 0; *args == ' ' && i < len; ++i, ++args);
  for (username = args; *args != '\0' && i < len; ++i, ++args) {
    if (*args == ' ') { *args = '\0'; i = len; }
  }
  len = strnlen(username, (size_t)(args - username));

  /* The remainder is residue */
  residue = (const char *)args;
  #ifndef NDEBUG
  if (*residue != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (argument residue)\n", residue);
  }
  #endif

  /* Prepare the statement and run the query */
  if (do_lookup(session_data.db_conn, &residue, username, len, &balance)) {
    fprintf(stderr, "ERROR: no account found for '%s'\n", username);
  } else {
    printf("%s's balance is $%li\n", username, balance);
  }
  #ifndef NDEBUG
  if (*residue != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (query residue)\n", residue);
  }
  #endif

  return BANKING_SUCCESS;
}

int
deposit_command(char * args)
{
  size_t len, i;
  char * username;
  const char * residue;
  long int amount, balance;

  /* Advance args to the first token, this is the username */
  len = strnlen(args, MAX_COMMAND_LENGTH);
  for (i = 0; *args == ' ' && i < len; ++i, ++args);
  for (username = args; *args != '\0' && i < len; ++i, ++args) {
    if (*args == ' ') { *args = '\0'; i = len; }
  }
  len = strnlen(username, (size_t)(args - username));

  /* We should now be at the addition amount */
  amount = strtol(args, (char **)(&residue), 10);
  #ifndef NDEBUG
  if (*residue != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (argument residue)\n", residue);
  }
  #endif

  /* Ensure the transaction amount is sane */
  if (amount < -MAX_TRANSACTION || amount > MAX_TRANSACTION) {
    fprintf(stderr, "ERROR: amount (%li) exceeds maximum transaction (%i)\n", amount, MAX_TRANSACTION);
    return BANKING_SUCCESS;
  }

  /* Prepare and run actual queries */
  if (do_lookup(session_data.db_conn, &residue, username, len, &balance)) {
    fprintf(stderr, "ERROR: no account found for '%s'\n", username);
  } else if (do_update(session_data.db_conn, &residue, username, len, balance + amount)) {
    fprintf(stderr, "ERROR: unable to complete request on ('%s', %li)\n", username, balance);
  } else {
    printf("A transaction of $%li brings %s's balance from $%li to $%li\n", amount, username, balance, balance + amount);
  }
  #ifndef NDEBUG
  if (*residue != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (query residue)\n", residue);
  }
  #endif

  return BANKING_SUCCESS;
}

void
handle_signal(int sig)
{
  size_t i;
  printf("\n");
  /* Perform a graceful shutdown of the system */
  session_data.caught_signal = sig;

  /* Send SIGTERM to every worker */
  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    if (session_data.tids[i] != (pthread_t)(BANKING_FAILURE)) {
      #ifndef NDEBUG
      fprintf(stderr, "INFO: sending SIGTERM to worker thread\n");
      #endif
      pthread_kill(session_data.tids[i], SIGTERM);
    }
  }

  /* Now collect them */
  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    if (session_data.tids[i] != (pthread_t)(BANKING_FAILURE)) {
      if (pthread_join(session_data.tids[i], NULL)) {
        fprintf(stderr, "ERROR: failed to collect worker thread\n");
      } else {
        #ifndef NDEBUG
        fprintf(stderr, "INFO: collected worker thread\n");
        #endif
        session_data.tids[i] = (pthread_t)(BANKING_SUCCESS);
      }
    }
  }

  /* Do remaining housekeeping */
  pthread_mutex_destroy(&session_data.accept_mutex);
  destroy_socket(session_data.sock);
  destroy_db(NULL, session_data.db_conn);
  if (sig == SIGINT) {
    signal(SIGINT, SIG_DFL);
    kill(getpid(), SIGINT);
  }
}

inline void
retire(int sig) {
  #ifndef NDEBUG
  fprintf(stderr, "INFO: worker thread retiring [signal %i]\n", sig);
  #endif
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
  #ifndef NDEBUG
  fprintf(stderr, "[thread %lu] INFO: worker started\n", *tid);
  #endif

  /* As long as possible, grab up whatever connection is available */
  while (!session_data.caught_signal) {
    pthread_mutex_lock(&session_data.accept_mutex);
    conn = accept(session_data.sock, (struct sockaddr *)(&remote_addr), &addr_len);
    pthread_mutex_unlock(&session_data.accept_mutex);
    if (conn >= 0) {
      #ifndef NDEBUG
      fprintf(stderr, "[thread %lu] INFO: worker connected to client\n", *tid);
      recv(conn, buffer, MAX_COMMAND_LENGTH, 0);
      buffer[MAX_COMMAND_LENGTH - 1] = '\0';
      fprintf(stderr, "[thread %lu] RECV: '%s'\n", *tid, buffer);
      #endif
      destroy_socket(conn);
    } else {
      fprintf(stderr, "[thread %lu] ERROR: worker failed to accept client\n", *tid);
      /* TODO wait, or break? */
    }
  }

  retire(0);
  return NULL;
}

int
main(int argc, char ** argv)
{
  int i;
  char * in, * args;
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
  if (init_db(":memory:", &session_data.db_conn)) {
    fprintf(stderr, "FATAL: failed to connect to database\n");
    return EXIT_FAILURE;
  }

  /* Thread initialization */
  session_data.caught_signal = 0;
  pthread_mutex_init(&session_data.accept_mutex, NULL);
  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    if (pthread_create(&session_data.tids[i], NULL, &handle_client, &session_data.tids[i])) {
      session_data.tids[i] = (pthread_t)(BANKING_FAILURE);
      fprintf(stderr, "WARNING: failed to start worker thread\n");
    }
  }
  signal(SIGINT, handle_signal);

  /* Issue an interactive prompt, only quit on signal */
  while (!session_data.caught_signal && (in = readline(SHELL_PROMPT))) {
    /* Catch invalid commands */
    if (validate(in, &cmd, &args)) {
      /* Ignore empty strings */
      if (*in != '\0') {
        fprintf(stderr, "ERROR: invalid command '%s'\n", in);
      }
    } else {
      /* Add the command to the shell history */
      add_history(in);
      /* Hook the command's return value to this signal */
      session_data.caught_signal = ((cmd == NULL) || cmd(args));
    }
    /* Cleanup from here down */
    free(in);
    in = NULL;
  }

  handle_signal(SIGTERM);
  return EXIT_SUCCESS;
}
