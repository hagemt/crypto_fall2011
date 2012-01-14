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
#define USING_PTHREADS
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
  struct credential_t credentials;
  struct buffet_t buffet;
  struct sigaction * signal_action;
  volatile int caught_signal;
  struct sockaddr_storage remote_addr;
  socklen_t remote_addr_len;
};

struct server_session_data_t {
  int sock;
  sqlite3 * db_conn;
  pthread_mutex_t * accept_mutex, * keystore_mutex;
  struct thread_data_t thread_data[MAX_CONNECTIONS];
  struct sigaction signal_action;
  volatile int caught_signal;
} session_data;

/* PROMPT COMMANDS *******************************************************/

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
    fprintf(stderr,
            "ERROR: amount (%li) exceeds maximum transaction (%i)\n",
            amount, MAX_TRANSACTION);
    return BANKING_SUCCESS;
  }

  /* Prepare and run actual queries */
  if (do_lookup(session_data.db_conn, &residue,
                username, len, &balance)) {
    fprintf(stderr, "ERROR: no account found for '%s'\n", username);
  } else if (do_update(session_data.db_conn, &residue,
                       username, len, balance + amount)) {
    fprintf(stderr,
            "ERROR: unable to complete request on ('%s', %li)\n",
            username, balance);
  } else {
    printf("A transaction of $%li brings %s's balance from $%li to $%li\n",
           amount, username, balance, balance + amount);
  }
  #ifndef NDEBUG
  if (*residue != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (query residue)\n", residue);
  }
  #endif

  return BANKING_SUCCESS;
}
#endif /* USE_DEPOSIT */

/* HANDLERS **************************************************************/

#ifdef HANDLE_LOGIN
int
handle_login_command(struct thread_data_t * datum, char * args)
{
  size_t i, len;
  char buffer[MAX_COMMAND_LENGTH];

  /* The login argument takes one argument */
  #ifndef NDEBUG
  if (*args == '\0') {
    fprintf(stderr,
            "[thread %lu] WARNING: [%s] arguments empty\n",
            datum->id, "login");
  } else {
    fprintf(stderr,
            "[thread %lu] INFO: [%s] '%s' (arguments)\n",
            datum->id, "login", args);
  }
  #endif
  /* TODO verify they're an actual user of the system */

  /* Modify the key using bits from the username */
  len = strnlen(args, MAX_COMMAND_LENGTH);
  for (i = 0; i < AUTH_KEY_LENGTH; ++i) {
    datum->credentials.key[i] ^= args[i % len];
  }
  /* Turn around the message */
  for (i = 0; i < MAX_COMMAND_LENGTH; ++i) {
    datum->buffet.pbuffer[i] =
     datum->buffet.tbuffer[MAX_COMMAND_LENGTH - 1 - i];
  }
  /* Echo this message with the modified key */
  encrypt_message(&datum->buffet, datum->credentials.key);
  send_message(&datum->buffet, datum->sock);

  /* Receive the PIN */
  recv_message(&datum->buffet, datum->sock);
  decrypt_message(&datum->buffet, datum->credentials.key);
  /* Copy the args backward TODO better auth */
  memset(buffer, '\0', MAX_COMMAND_LENGTH);
  for (i = 0; i < len; ++i) {
    buffer[i] = args[len - i - 1];
  }

  /* Check the buffer matches the args */
  if (strncmp(buffer, datum->buffet.tbuffer, len)
   || do_lookup(session_data.db_conn, NULL, args, len, NULL)) {
    snprintf(buffer, MAX_COMMAND_LENGTH, "LOGIN ERROR");
    /* Remove the previously added bits */
    for (i = 0; i < AUTH_KEY_LENGTH; ++i) {
      datum->credentials.key[i] ^= args[i % len];
    }
  } else {
    snprintf(buffer, MAX_COMMAND_LENGTH, "%s, %s!", AUTH_LOGIN_MSG, args);
    /* We have now authenticated the user */
    memset(datum->credentials.username, '\0', MAX_COMMAND_LENGTH);
    strncpy(datum->credentials.username, args, len);
    datum->credentials.userlength = len;
  }
  salt_and_pepper(buffer, NULL, &datum->buffet);
  encrypt_message(&datum->buffet, datum->credentials.key);
  send_message(&datum->buffet, datum->sock);

  /* Catch authentication check */
  recv_message(&datum->buffet, datum->sock);
  decrypt_message(&datum->buffet, datum->credentials.key);
  /* Turn around the message */
  for (i = 0; i < MAX_COMMAND_LENGTH; ++i) {
    datum->buffet.pbuffer[i] =
     datum->buffet.tbuffer[MAX_COMMAND_LENGTH - 1 - i];
  }
  encrypt_message(&datum->buffet, datum->credentials.key);
  send_message(&datum->buffet, datum->sock);

  return BANKING_SUCCESS;
}
#endif /* HANDLE_LOGIN */

#ifdef HANDLE_BALANCE
int
handle_balance_command(struct thread_data_t * datum, char * args)
{
  long int balance;
  char buffer[MAX_COMMAND_LENGTH];

  /* Balance command takes no arguments */
  #ifndef NDEBUG
  if (*args != '\0') {
    fprintf(stderr,
            "[thread %lu] WARNING: ignoring '%s' (argument residue)\n",
            datum->id, args);
  }
  #endif

  /* Formulate a response */
  memset(buffer, '\0', MAX_COMMAND_LENGTH);
  /* If we have a username, try to do a lookup */
  if (datum->credentials.userlength
   && do_lookup(session_data.db_conn, NULL,
                datum->credentials.username,
                datum->credentials.userlength,
                &balance) == BANKING_SUCCESS) {
    snprintf(buffer, MAX_COMMAND_LENGTH,
             "%s, your balance is $%li.",
             datum->credentials.username, balance);
  } else {
    snprintf(buffer, MAX_COMMAND_LENGTH, "BALANCE ERROR");
  }
  /* Send the results */
  salt_and_pepper(buffer, NULL, &datum->buffet);
  encrypt_message(&datum->buffet, datum->credentials.key);
  send_message(&datum->buffet, datum->sock);
  return BANKING_SUCCESS;
}
#endif /* HANDLE_BALANCE */

#ifdef HANDLE_WITHDRAW
int
handle_withdraw_command(struct thread_data_t * datum, char * args)
{
  long balance, amount;
  char buffer[MAX_COMMAND_LENGTH];

  #ifndef NDEBUG
  if (*args == '\0') {
    fprintf(stderr,
            "[thread %lu] WARNING: [%s] arguments empty\n",
            datum->id, "withdraw");
  } else {
    fprintf(stderr,
            "[thread %lu] INFO: [%s] '%s' (arguments)\n",
            datum->id, "withdraw", args);
  }
  #endif
  amount = strtol(args, &args, 10);

  if (amount <= 0 || amount > MAX_TRANSACTION) {
    snprintf(buffer, MAX_COMMAND_LENGTH, "Invalid withdrawal amount.");
  } else if (datum->credentials.userlength
   && do_lookup(session_data.db_conn, NULL,
                datum->credentials.username,
                datum->credentials.userlength,
                &balance) == BANKING_SUCCESS) {
    if (balance < amount) {
      snprintf(buffer, MAX_COMMAND_LENGTH, "Insufficient funds.");
    } else if (do_update(session_data.db_conn, NULL,
               datum->credentials.username,
               datum->credentials.userlength,
               balance - amount)) {
      snprintf(buffer, MAX_COMMAND_LENGTH, "Cannot complete withdrawal.");
    } else {
      snprintf(buffer, MAX_COMMAND_LENGTH, "Withdrew $%li", amount);
    }
  } else {
    snprintf(buffer, MAX_COMMAND_LENGTH, "WITHDRAW ERROR");
  }
  salt_and_pepper(buffer, NULL, &datum->buffet);
  encrypt_message(&datum->buffet, datum->credentials.key);
  send_message(&datum->buffet, datum->sock);

  return BANKING_SUCCESS;
}
#endif /* HANDLE_WITHDRAW */

#ifdef HANDLE_LOGOUT
int
handle_logout_command(struct thread_data_t * datum, char * args)
{
  int i, len;
  char buffer[MAX_COMMAND_LENGTH];

  /* Logout command takes no arguments */
  #ifndef NDEBUG
  if (*args != '\0') {
    fprintf(stderr,
            "[thread %lu] WARNING: ignoring '%s' (argument residue)\n",
            datum->id, args);
  }
  #endif

  /* Prepare a reply dependent on our state */
  memset(buffer, '\0', MAX_COMMAND_LENGTH);
  if (datum->credentials.userlength) {
    snprintf(buffer, MAX_COMMAND_LENGTH, "Goodbye, %s!",
             datum->credentials.username);
  } else {
    snprintf(buffer, MAX_COMMAND_LENGTH, "LOGOUT ERROR");
  }
  salt_and_pepper(buffer, NULL, &datum->buffet);
  encrypt_message(&datum->buffet, datum->credentials.key);
  /* Clear the credential bits from the key */
  if (datum->credentials.userlength) {
    len = datum->credentials.userlength;
    for (i = 0; i < AUTH_KEY_LENGTH; ++i) {
      datum->credentials.key[i] ^=
       datum->credentials.username[i % len];
    }
    memset(&datum->credentials.username, '\0', MAX_COMMAND_LENGTH);
    datum->credentials.userlength = 0;
  }
  send_message(&datum->buffet, datum->sock);

  /* Handle verification (should fail) */
  recv_message(&datum->buffet, datum->sock);
  decrypt_message(&datum->buffet, datum->credentials.key);
  for (i = 0; i < MAX_COMMAND_LENGTH; ++i) {
    datum->buffet.pbuffer[i] =
     datum->buffet.tbuffer[MAX_COMMAND_LENGTH - 1 - i];
  }
  encrypt_message(&datum->buffet, datum->credentials.key);
  send_message(&datum->buffet, datum->sock);

  return BANKING_SUCCESS;
}
#endif /* HANDLE_LOGOUT */

#ifdef HANDLE_TRANSFER
int
handle_transfer_command(struct thread_data_t * datum, char * args)
{
  long amount, balance;
  char * user, buffer[MAX_COMMAND_LENGTH];

  #ifndef NDEBUG
  if (*args == '\0') {
    fprintf(stderr,
            "[thread %lu] WARNING: [%s] arguments empty\n",
            datum->id, "transfer");
  } else {
    fprintf(stderr,
            "[thread %lu] INFO: [%s] '%s' (arguments)\n",
            datum->id, "transfer", args);
  }
  #endif
  amount = strtol(args, &args, 10);
  user = ++args;

  if (amount <= 0 || amount > MAX_TRANSACTION) {
    snprintf(buffer, MAX_COMMAND_LENGTH, "Invalid transfer amount.");
  } else if (datum->credentials.userlength
   && do_lookup(session_data.db_conn, NULL,
                datum->credentials.username,
                datum->credentials.userlength,
                &balance) == BANKING_SUCCESS) {
    if (balance < amount) {
      snprintf(buffer, MAX_COMMAND_LENGTH, "Insufficient funds.");
    } else if (do_update(session_data.db_conn, NULL,
               datum->credentials.username,
               datum->credentials.userlength,
               balance - amount)
            || do_lookup(session_data.db_conn, NULL,
               user, strnlen(user, MAX_COMMAND_LENGTH),
               &balance)
            || do_update(session_data.db_conn, NULL,
               user, strnlen(user, MAX_COMMAND_LENGTH),
               balance + amount)) {
      /* TODO atomic operation? */
      snprintf(buffer, MAX_COMMAND_LENGTH, "Cannot complete transfer.");
    } else {
      snprintf(buffer, MAX_COMMAND_LENGTH, "Transfered $%li to %s",
                                           amount, user);
    }
  } else {
    snprintf(buffer, MAX_COMMAND_LENGTH, "TRANSFER ERROR");
  }
  salt_and_pepper(buffer, NULL, &datum->buffet);
  encrypt_message(&datum->buffet, datum->credentials.key);
  send_message(&datum->buffet, datum->sock);

  return BANKING_SUCCESS;
}
#endif /* HANDLE_TRANSFER */

/* SIGNAL HANDLERS *******************************************************/

void
handle_signal(int signum)
{
  int i;
  putchar('\n');
  if (signum) {
    fprintf(stderr,
            "WARNING: signal caught [code %i: %s]\n",
            signum, strsignal(signum));
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
  gcry_pthread_mutex_destroy((void **)(&session_data.accept_mutex));
  gcry_pthread_mutex_destroy((void **)(&session_data.keystore_mutex));
  destroy_socket(session_data.sock);
  /* TODO remove shared memory code */
  shutdown_crypto(old_shmid(&i));
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
    fprintf(stderr,
            "WARNING: worker thread caught signal [code %i: %s]\n",
            signum, strsignal(signum));
  }
}

/* CLIENT HANDLERS *******************************************************/

/*! \brief Handle a message stream from a client */
int
handle_stream(struct thread_data_t * datum) {
  int i;
  handle_t hdl;
  char msg[MAX_COMMAND_LENGTH], * args;

  /* The initial message is always an authentication request */
  recv_message(&datum->buffet, datum->sock);
  decrypt_message(&datum->buffet, datum->credentials.key);
  if (strncmp(datum->buffet.tbuffer,
              AUTH_CHECK_MSG, sizeof(AUTH_CHECK_MSG))) {
    #ifndef NDEBUG
    fprintf(stderr,
            "[thread %lu] INFO: malformed authentication message\n",
            datum->id);
    #endif
    /* Respond with a "mumble" (nonce) */
    gcry_create_nonce(datum->buffet.pbuffer, MAX_COMMAND_LENGTH);
    encrypt_message(&datum->buffet, datum->credentials.key);
    send_message(&datum->buffet, datum->sock);
    return BANKING_FAILURE;
  }
  /* Turn around the buffer and toss it back */
  for (i = 0; i < MAX_COMMAND_LENGTH; ++i) {
    datum->buffet.pbuffer[i] =
     datum->buffet.tbuffer[MAX_COMMAND_LENGTH - 1 - i];
  }
  encrypt_message(&datum->buffet, datum->credentials.key);
  send_message(&datum->buffet, datum->sock);
  clear_buffet(&datum->buffet);
  #ifndef NDEBUG
  fprintf(stderr,
          "[thread %lu] INFO: authentication successful\n",
          datum->id);
  #endif

  /* Read the actual command */
  recv_message(&datum->buffet, datum->sock);
  decrypt_message(&datum->buffet, datum->credentials.key);
  /* Copy the command into a buffer so more can be received */
  strncpy(msg, datum->buffet.tbuffer, MAX_COMMAND_LENGTH);
  #ifndef NDEBUG
  /* Local echo for all received messages */
  fprintf(stderr,
          "[thread %lu] INFO: worker received message:\n",
          datum->id);
  hexdump(stderr, (unsigned char *)(msg), MAX_COMMAND_LENGTH);
  #endif
  
  /* Disconnect from any client that issues malformed commands */
  if (fetch_handle(msg, &hdl, &args)) {
    /* Respond with a "mumble" (nonce) */
    gcry_create_nonce(datum->buffet.pbuffer, MAX_COMMAND_LENGTH);
    encrypt_message(&datum->buffet, datum->credentials.key);
    send_message(&datum->buffet, datum->sock);
    clear_buffet(&datum->buffet);
    return BANKING_FAILURE;
  }
  /* We are signaled by failed handlers */
  datum->caught_signal = hdl(datum, args);
  clear_buffet(&datum->buffet);

  return BANKING_SUCCESS;
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
    /* Ensure only one worker accepts the next client */
    gcry_pthread_mutex_lock((void **)(&session_data.accept_mutex));
    datum->sock = accept(session_data.sock,
                         (struct sockaddr *)(&datum->remote_addr),
                         &datum->remote_addr_len);
    gcry_pthread_mutex_unlock((void **)(&session_data.accept_mutex));
    if (datum->sock >= 0) {
      #ifndef NDEBUG
      fprintf(stderr,
              "[thread %lu] INFO: worker connected to client\n",
              datum->id);
      #endif
      /* Receive a "hello" message from the client */
      recv_message(&datum->buffet, datum->sock);
      /* Decrypt it with the default key */
      decrypt_message(&datum->buffet, keystore.key);
      /* Verify it is an authentication request */
      if (strncmp(datum->buffet.tbuffer,
                  AUTH_CHECK_MSG, sizeof(AUTH_CHECK_MSG))) {
        /* Respond with nonce (misdirection) */
        gcry_create_nonce(datum->buffet.pbuffer, MAX_COMMAND_LENGTH);
        encrypt_message(&datum->buffet, keystore.key);
        send_message(&datum->buffet, datum->sock);
      } else {
        /* Request a session key */
        gcry_pthread_mutex_lock((void **)(&session_data.keystore_mutex));
        #ifndef NDEBUG
        print_keystore(stderr, "before request");
        #endif
        request_key(&datum->credentials.key);
        #ifndef NDEBUG
        print_keystore(stderr, "after request");
        #endif
        gcry_pthread_mutex_unlock((void **)(&session_data.keystore_mutex));
        /* Encrypted it using the default key */
        salt_and_pepper((char *)(datum->credentials.key), NULL,
                        &datum->buffet);
        encrypt_message(&datum->buffet, keystore.key);
        send_message(&datum->buffet, datum->sock);
        clear_buffet(&datum->buffet);
        /* Repeatedly poll for message streams */
        while (handle_stream(datum) == BANKING_SUCCESS);
        /* Revoke the session key */
        gcry_pthread_mutex_lock((void **)(&session_data.keystore_mutex));
        #ifndef NDEBUG
        print_keystore(stderr, "before revoke");
        #endif
        revoke_key(&datum->credentials.key);
        #ifndef NDEBUG
        print_keystore(stderr, "after revoke");
        #endif
        gcry_pthread_mutex_unlock((void **)(&session_data.keystore_mutex));
      }
      /* Cleanup (disconnect) */
      #ifndef NDEBUG
      fprintf(stderr,
              "[thread %lu] INFO: worker disconnected from client\n",
              datum->id);
      #endif
      clear_buffet(&datum->buffet);
      destroy_socket(datum->sock);
      datum->sock = BANKING_FAILURE;
    } else {
      fprintf(stderr,
              "[thread %lu] ERROR: worker unable to connect\n",
              datum->id);
    }
  }

  /* Teardown */
  handle_interruption(0);
  return NULL;
}

int
main(int argc, char ** argv)
{
  command_t cmd; int i;
  char * in, * args, buffer[MAX_COMMAND_LENGTH];
  struct sigaction thread_signal_action, old_signal_action;
  struct thread_data_t * thread_datum;

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
  gcry_pthread_mutex_init((void **)(&session_data.accept_mutex));
  gcry_pthread_mutex_init((void **)(&session_data.keystore_mutex));
  /* Save the old list of blocked signals for later */
  pthread_sigmask(SIG_SETMASK, NULL, &old_signal_action.sa_mask);
  /* Worker threads inherit this mask (ignore everything except SIGUSRs) */
  memset(&thread_signal_action, '\0', sizeof(struct sigaction));
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
    memset(thread_datum, '\0', sizeof(struct thread_data_t));
    thread_datum->sock = BANKING_FAILURE;
    thread_datum->remote_addr_len = sizeof(thread_datum->remote_addr);
    thread_datum->signal_action = &thread_signal_action;
    if (pthread_create(&thread_datum->id, NULL, &handle_client,
                                                thread_datum)) {
      thread_datum->id = (pthread_t)(BANKING_FAILURE);
      fprintf(stderr, "WARNING: unable to start worker thread\n");
    }
  }
  /* Reset the signal mask to the prior behavior, and ignore SIGUSRs */
  sigaddset(&old_signal_action.sa_mask, SIGUSR1);
  sigaddset(&old_signal_action.sa_mask, SIGUSR2);
  pthread_sigmask(SIG_SETMASK, &old_signal_action.sa_mask, NULL);

  /* Session Signal initialization */
  memset(&session_data.signal_action, '\0', sizeof(struct sigaction));
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
  session_data.caught_signal = 0;
  /* TODO tab-completion for commands */
  rl_bind_key('\t', rl_insert);

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
        rl_ding();
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

