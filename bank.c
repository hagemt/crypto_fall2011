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
#define HANDLE_LOGIN
#define HANDLE_BALANCE
#define HANDLE_WITHDRAW
#define HANDLE_LOGOUT
#define HANDLE_TRANSFER
#include "banking_commands.h"
#include "banking_constants.h"
#include "crypto_utils.h"
#include "db_utils.h"
#include "socket_utils.h"

struct thread_data_t {
  pthread_t id;
  int sock;
  char buffer[MAX_COMMAND_LENGTH];
  struct sigaction * signal_action;
  volatile int caught_signal;
  struct sockaddr_storage remote_addr;
  socklen_t remote_addr_len;
};

struct server_session_data_t {
  int sock;
  sqlite3 * db_conn;
  pthread_mutex_t accept_mutex;
  struct thread_data_t thread_data[MAX_CONNECTIONS];
  struct sigaction signal_action;
  volatile int caught_signal;
} session_data;

/* PROMPT COMMANDS ***********************************************************/

#ifdef USE_BALANCE
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
  residue = (const char *)(args);
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
#endif /* USE_BALANCE */

#ifdef USE_DEPOSIT
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

  /* Ensure the transaction amount is sane, TODO better solution? */
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
#endif /* USE_DEPOSIT */

/* HANDLERS ******************************************************************/

#ifdef HANDLE_LOGIN
int
handle_login_command(char * args)
{
  fprintf(stderr, "INFO: login handled '%s' (argument residue)\n", args);
  return BANKING_SUCCESS;
}
#endif /* HANDLE_LOGIN */

#ifdef HANDLE_BALANCE
int
handle_balance_command(char * args)
{
  fprintf(stderr, "INFO: balance handled '%s' (argument residue)\n", args);
  return BANKING_SUCCESS;
}
#endif /* HANDLE_BALANCE */

#ifdef HANDLE_WITHDRAW
int
handle_withdraw_command(char * args)
{
  fprintf(stderr, "INFO: withdraw handled '%s' (argument residue)\n", args);
  return BANKING_SUCCESS;
}
#endif /* HANDLE_WITHDRAW */

#ifdef HANDLE_LOGOUT
int
handle_logout_command(char * args)
{
  fprintf(stderr, "INFO: logout handled '%s' (argument residue)\n", args);
  return BANKING_SUCCESS;
}
#endif /* HANDLE_LOGOUT */

#ifdef HANDLE_TRANSFER
int
handle_transfer_command(char * args)
{
  fprintf(stderr, "INFO: transfer handled '%s' (argument residue)\n", args);
  return BANKING_SUCCESS;
}
#endif /* HANDLE_TRANSFER */

/* DRIVERS *******************************************************************/

void
handle_signal(int signum)
{
  int i;
  printf("\n");
  if (signum) {
    fprintf(stderr, "WARNING: signal caught [code %i: %s]\n", signum, strsignal(signum));
  }
  /* Perform a graceful shutdown of the system */
  session_data.caught_signal = signum;

  /* Send SIGTERM to every worker */
  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    if (session_data.thread_data[i].id != (pthread_t)(BANKING_FAILURE)) {
      #ifndef NDEBUG
      fprintf(stderr, "INFO: sending kill signal to worker thread\n");
      #endif
      pthread_kill(session_data.thread_data[i].id, SIGUSR1);
    }
  }

  /* Now collect them */
  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    if (session_data.thread_data[i].id != (pthread_t)(BANKING_FAILURE)) {
      if (pthread_join(session_data.thread_data[i].id, NULL)) {
        session_data.thread_data[i].id = (pthread_t)(BANKING_FAILURE);
        fprintf(stderr, "ERROR: failed to collect worker thread\n");
      } else {
        session_data.thread_data[i].id = (pthread_t)(BANKING_SUCCESS);
        #ifndef NDEBUG
        fprintf(stderr, "INFO: collected worker thread\n");
        #endif
      }
    }
  }

  /* Do remaining housekeeping */
  pthread_mutex_destroy(&session_data.accept_mutex);
  destroy_socket(session_data.sock);
  /* TODO remove shared memory code */
  old_shmid(&i);
  shutdown_crypto(&i);
  if (shmctl(i, IPC_RMID, NULL)) {
    fprintf(stderr, "WARNING: unable to remove shared memory segment\n");
  }
  destroy_db(NULL, session_data.db_conn);

  /* Re-raise proper signals */
  if (signum == SIGINT || signum == SIGTERM) {
    sigemptyset(&session_data.signal_action.sa_mask);
    session_data.signal_action.sa_handler = SIG_DFL;
    sigaction(signum, &session_data.signal_action, NULL);
    raise(signum);
  }
}

void
handle_interruption(int signum)
{
  if (signum == SIGUSR1) {
    #ifndef NDEBUG
    fprintf(stderr, "INFO: worker thread retiring\n");
    #endif
    pthread_exit(NULL);
  } else {
    fprintf(stderr, "WARNING: signal caught [code %i: %s]\n", signum, strsignal(signum));
  }
}

void *
handle_client(void * arg)
{
  struct thread_data_t * datum = (struct thread_data_t *)(arg);
  /* Fetch the ID from the argument */
  #ifndef NDEBUG
  fprintf(stderr, "[thread %lu] INFO: worker started\n", datum->id);
  #endif

  /* Worker thread signal handling is unique */
  sigaction(SIGUSR1, datum->signal_action, NULL);
  sigaction(SIGUSR2, datum->signal_action, NULL);

  /* As long as possible, grab up whatever connection is available */
  while (!session_data.caught_signal && !datum->caught_signal) {
    pthread_mutex_lock(&session_data.accept_mutex);
    datum->sock = accept(session_data.sock, (struct sockaddr *)(&datum->remote_addr), &datum->remote_addr_len);
    pthread_mutex_unlock(&session_data.accept_mutex);
    if (datum->sock >= 0) {
      #ifndef NDEBUG
      fprintf(stderr, "[thread %lu] INFO: worker connected to client\n", datum->id);
      memset(datum->buffer, '\0', MAX_COMMAND_LENGTH);
      recv(datum->sock, datum->buffer, MAX_COMMAND_LENGTH, 0);
      datum->buffer[MAX_COMMAND_LENGTH - 1] = '\0';
      fprintf(stderr, "[thread %lu] RECV: '%s'\n", datum->id, datum->buffer);
      #endif
      destroy_socket(datum->sock);
      datum->sock = BANKING_FAILURE;
    } else {
      fprintf(stderr, "[thread %lu] ERROR: worker failed to accept client\n", datum->id);
    }
  }

  /* Teardown */
  handle_interruption(0);
  return NULL;
}

int
main(int argc, char ** argv)
{
  char * in, * args, buffer[MAX_COMMAND_LENGTH];
  struct sigaction thread_signal_action, old_signal_action;
  struct thread_data_t * thread_datum;
  command cmd;
  int i;

  /* Sanitize input */
  if (argc != 2) {
    fprintf(stderr, "USAGE: %s port\n", argv[0]);
    return EXIT_FAILURE;
  }

  /* Crypto initialization */
  if (init_crypto(new_shmid(&i))) {
    fprintf(stderr, "FATAL: unable to enter secure mode\n");
    return EXIT_FAILURE;
  }

  /* Database initialization */
  if (init_db(":memory:", &session_data.db_conn)) {
    fprintf(stderr, "FATAL: unable to connect to database\n");
    shutdown_crypto(old_shmid(&i));
    return EXIT_FAILURE;
  }

  /* Socket initialization */
  if ((session_data.sock = init_server_socket(argv[1])) < 0) {
    fprintf(stderr, "FATAL: unable to start server\n");
    destroy_db(NULL, session_data.db_conn);
    shutdown_crypto(old_shmid(&i));
    return EXIT_FAILURE;
  }

  /* Thread initialization */
  pthread_mutex_init(&session_data.accept_mutex, NULL);
  /* Save the old list of blocked signals for later */
  pthread_sigmask(SIG_SETMASK, NULL, &old_signal_action.sa_mask);
  /* Worker threads inherit a signal mask that ignores everything except SIGUSRs */
  memset(&thread_signal_action, '\0', sizeof(thread_signal_action));
  sigfillset(&thread_signal_action.sa_mask);
  sigdelset(&thread_signal_action.sa_mask, SIGUSR1);
  sigdelset(&thread_signal_action.sa_mask, SIGUSR2);
  thread_signal_action.sa_handler = &handle_interruption;
  pthread_sigmask(SIG_SETMASK, &thread_signal_action.sa_mask, NULL);
  /* Afterwhich, all signals should be ignored in the handler */
  sigfillset(&thread_signal_action.sa_mask);
  /* Kick off the workers */
  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    /* Thread data initialization */
    thread_datum = &session_data.thread_data[i];
    thread_datum->remote_addr_len = sizeof(thread_datum->remote_addr);
    thread_datum->signal_action = &thread_signal_action;
    if (pthread_create(&thread_datum->id, NULL, &handle_client, thread_datum)) {
      thread_datum->id = (pthread_t)(BANKING_FAILURE);
      fprintf(stderr, "WARNING: unable to start worker thread\n");
    }
  }
  /* Reset the thread signal mask to the prior behavior, ignoring SIGUSRs */
  sigaddset(&old_signal_action.sa_mask, SIGUSR1);
  sigaddset(&old_signal_action.sa_mask, SIGUSR2);
  pthread_sigmask(SIG_SETMASK, &old_signal_action.sa_mask, NULL);

  /* Session Signal initialization */
  session_data.caught_signal = 0;
  memset(&session_data.signal_action, '\0', sizeof(session_data.signal_action));
  /* The signal handler should ignore SIGTERM and SIGINT */
  sigemptyset(&session_data.signal_action.sa_mask);
  sigaddset(&session_data.signal_action.sa_mask, SIGTERM);
  sigaddset(&session_data.signal_action.sa_mask, SIGINT);
  session_data.signal_action.sa_handler = &handle_signal;
  /* Make sure any ignored signals remain ignored */
  sigaction(SIGTERM, NULL, &old_signal_action);
  if (old_signal_action.sa_handler != SIG_IGN) {
    sigaction(SIGTERM, &session_data.signal_action, NULL);
  }
  sigaction(SIGINT, NULL, &old_signal_action);
  if (old_signal_action.sa_handler != SIG_IGN) {
    sigaction(SIGINT, &session_data.signal_action, NULL);
  }

  /* Issue an interactive prompt, only quit on signal */
  while (!session_data.caught_signal && (in = readline(SHELL_PROMPT))) {
    /* Ignore empty strings */
    if (*in != '\0') {
      /* Add the original command to the shell history */
      memset(buffer, '\0', MAX_COMMAND_LENGTH);
      strncpy(buffer, in, MAX_COMMAND_LENGTH);
      buffer[MAX_COMMAND_LENGTH - 1] = '\0';
      for (i = 0; buffer[i] == ' '; ++i);
      add_history(buffer + i);
      /* Catch invalid commands prior to invocation */
      if (validate_command(in, &cmd, &args)) {
        fprintf(stderr, "ERROR: invalid command '%s'\n", in);
      } else {
        /* Hook the command's return value to this signal */
        session_data.caught_signal = ((cmd == NULL) || cmd(args));
      }
    }
    free(in);
    in = NULL;
  }

  /* Teardown */
  handle_signal(0);
  return EXIT_SUCCESS;
}

