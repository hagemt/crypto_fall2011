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
#include <string.h>

/* GNU includes */
#include <readline/readline.h>
#include <readline/history.h>

/* UNIX includes */
#include <termios.h>
#include <unistd.h>

/* Local includes */
#define USE_LOGIN
#define USE_BALANCE
#define USE_WITHDRAW
#define USE_LOGOUT
#define USE_TRANSFER
#include "banking_commands.h"
#include "banking_constants.h"
#include "crypto_utils.h"
#include "socket_utils.h"

/* SESSION DATA **************************************************************/

/* TODO do better saving of username, w/ salting? */
struct client_session_data_t {
  int sock;
  char * user;
  unsigned char * key;
  struct buffet_t buffet;
  struct termios terminal_state;
} session_data;

int
authenticated(struct client_session_data_t * session)
{
  int status = BANKING_FAILURE;
  if (session) {
    if (!session->key) {
      request_key(&session->key);
      status = BANKING_SUCCESS;
    }
    salt_and_pepper(AUTH_CHECK_MSG, NULL, &session->buffet);
    encrypt_command(&session->buffet, session->key);
    put_message(&session->buffet, session->sock);
    get_message(&session->buffet, session->sock);
    decrypt_command(&session->buffet, session->key);
    if (status == BANKING_SUCCESS) {
      revoke_key(session->key);
    }
    status = cmpbuffet(&session->buffet);
    clrbuffet(&session->buffet);
  }
  return status;
}

int
acquire_credentials(struct termios * terminal_state, char ** pin)
{
  if (tcgetattr(0, terminal_state)) {
    #ifndef NDEBUG
    fprintf(stderr, "ERROR: unable to determine terminal attributes\n");
    #endif
    return BANKING_FAILURE;
  }

  terminal_state->c_lflag &= ~ECHO;
  if (tcsetattr(0, TCSANOW, terminal_state)) {
    #ifndef NDEBUG
    fprintf(stderr, "ERROR: unable to disable terminal echo\n");
    #endif
    return BANKING_FAILURE;
  }

  *pin = readline(PIN_PROMPT);
  putchar('\n');

  /* TODO signal handler for ensuring ECHO is re-enabled? */
  terminal_state->c_lflag |= ECHO;
  if (tcsetattr(0, TCSANOW, terminal_state)) {
    #ifndef NDEBUG
    fprintf(stderr, "ERROR: unable to re-enable terminal echo\n");
    #endif
  }
  
  return (*pin) ? BANKING_SUCCESS : BANKING_FAILURE;
}

/* COMMANDS ******************************************************************/

#ifdef USE_LOGIN
int
login_command(char * args)
{
  size_t i, len;
  char * pin, buffer[MAX_COMMAND_LENGTH];

  /* Input sanitation */
  len = strnlen(args, MAX_COMMAND_LENGTH);
  /* Advance to the first non-space */
  for (i = 0; *args == ' ' && i < len; ++i, ++args);
  for (session_data.user = args; *args != '\0' && i < len; ++i, ++args) {
    if (*args == ' ') { *args = '\0'; i = len; }
  }
  #ifndef NDEBUG
  if (*args != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (argument residue)\n", args);
  }
  #endif

  /* Only non-authenticated users may login */
  if (authenticated(&session_data) == BANKING_FAILURE) {
    /* Request a key and send the message "login [username]" */
    request_key(&session_data.key);
    memset(buffer, '\0', MAX_COMMAND_LENGTH);
    snprintf(buffer, MAX_COMMAND_LENGTH, "login %s", session_data.user);
    salt_and_pepper(buffer, NULL, &session_data.buffet);
    encrypt_command(&session_data.buffet, session_data.key);
    put_message(&session_data.buffet, session_data.sock);
    /* Fetch the PIN from the user, and send that next */
    if (acquire_credentials(&session_data.terminal_state, &pin) == BANKING_SUCCESS) {
      salt_and_pepper(pin, NULL, &session_data.buffet);
      encrypt_command(&session_data.buffet, session_data.key);
      put_message(&session_data.buffet, session_data.sock);
      /* Wipe the buffer TODO noncify instead? safe? */
      memset(pin, '\0', strlen(pin));
      free(pin);
    }
    /* If we are not authenticated now, there is a problem */
    if (authenticated(&session_data) == BANKING_FAILURE) {
      revoke_key(session_data.key);
      printf("AUTHENTICATION FAILURE\n");
    }
    /* Wipe all the buffers */
    clrbuffet(&session_data.buffet);
  } else {
    printf("You must 'logout' first.\n");
  }

  return BANKING_SUCCESS;
}
#endif /* USE_LOGIN */

#ifdef USE_BALANCE
int
balance_command(char * args)
{
  /* Balance command takes no arguments, no input sanitation required */
  #ifndef NDEBUG
  if (*args != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (argument residue)\n", args);
  }
  #endif

  /* Users must first authenticate to check balances */
  if (authenticated(&session_data) == BANKING_SUCCESS) {
    salt_and_pepper("balance", NULL, &session_data.buffet);
    encrypt_command(&session_data.buffet, session_data.key);
    put_message(&session_data.buffet, session_data.sock);
    get_message(&session_data.buffet, session_data.sock);
    decrypt_command(&session_data.buffet, session_data.key);
    printf("%s's balance is: %s\n", session_data.user, strbuffet(&session_data.buffet));
    clrbuffet(&session_data.buffet);
  } else {
    printf("You must 'login' first.\n");
  }

  return BANKING_SUCCESS;
}
#endif /* USE_BALANCE */

#ifdef USE_WITHDRAW
int
withdraw_command(char * args)
{
  size_t i, len;
  long amount;
  char buffer[MAX_COMMAND_LENGTH];

  /* Input sanitation */
  len = strnlen(args, MAX_COMMAND_LENGTH);
  /* Advance to the first non-space */
  for (i = 0; *args == ' ' && i < len; ++i, ++args);
  /* TODO Check for an acceptable value */
  amount = strtol(args, &args, 10);
  #ifndef NDEBUG
  if (*args != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (argument residue)\n", args);
  }
  #endif

  /* Only authenticated users may perform withdrawals */
  if (authenticated(&session_data) == BANKING_SUCCESS) {
    /* Send the message "withdraw [amount]" */
    memset(buffer, '\0', MAX_COMMAND_LENGTH);
    snprintf(buffer, MAX_COMMAND_LENGTH, "withdraw %li", amount);
    salt_and_pepper(buffer, NULL, &session_data.buffet);
    encrypt_command(&session_data.buffet, session_data.key);
    put_message(&session_data.buffet, session_data.sock);
    get_message(&session_data.buffet, session_data.sock);
    decrypt_command(&session_data.buffet, session_data.key);
    printf("%s\n", strbuffet(&session_data.buffet));
    clrbuffet(&session_data.buffet);
  } else {
    printf("You must 'login' first.\n");
  }

  return BANKING_SUCCESS;
}
#endif /* USE_WITHDRAW */

#ifdef USE_LOGOUT
int
logout_command(char * args)
{
  /* Logout command takes no arguments, no input sanitation required */
  #ifndef NDEBUG
  if (*args != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (argument residue)\n", args);
  }
  #endif

  /* Only authenticated users can logout */
  if (authenticated(&session_data) == BANKING_SUCCESS) {
    salt_and_pepper("logout", NULL, &session_data.buffet);
    encrypt_command(&session_data.buffet, session_data.key);
    put_message(&session_data.buffet, session_data.sock);
    revoke_key(session_data.key);
    clrbuffet(&session_data.buffet);
  } else {
    printf("You must 'login' first.\n");
  }
 
  /* Ensure we are now not authenticated */
  if (authenticated(&session_data) == BANKING_SUCCESS) {
    fprintf(stderr, "MISAUTHORIZATION DETECTED\n");
  }
  
  return BANKING_SUCCESS;
}
#endif /* USE_LOGOUT */

#ifdef USE_TRANSFER
int
transfer_command(char * args)
{
  size_t i, len;
  long amount;
  char buffer[MAX_COMMAND_LENGTH], * username;

  /* Input sanitation */
  len = strnlen(args, MAX_COMMAND_LENGTH);
  /* Advance to the first non-space */
  for (i = 0; *args == ' ' && i < len; ++i, ++args);
  /* Check for an acceptable value TODO safe? */
  amount = strtol(args, &args, 10);
  /* Advance to the next non-space, and snag the username */
  len = strnlen(args, MAX_COMMAND_LENGTH);
  for (i = 0; *args == ' ' && i < len; ++i, ++args);
  for (username = args; *args != '\0' && i < len; ++i, ++args) {
    if (*args == ' ') { *args = '\0'; i = len; }
  }
  #ifndef NDEBUG
  if (*args != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (argument residue)\n", args);
  }
  #endif

  /* Only authenticated users may authorize transfers */
  if (authenticated(&session_data) == BANKING_SUCCESS) {
    /* Send the command "transfer [amount] [recipient]" */
    memset(buffer, '\0', MAX_COMMAND_LENGTH);
    snprintf(buffer, MAX_COMMAND_LENGTH, "transfer %li %s", amount, username);
    salt_and_pepper(buffer, NULL, &session_data.buffet);
    encrypt_command(&session_data.buffet, session_data.key);
    put_message(&session_data.buffet, session_data.sock);
    get_message(&session_data.buffet, session_data.sock);
    decrypt_command(&session_data.buffet, session_data.key);
    printf("%s\n", strbuffet(&session_data.buffet));
    clrbuffet(&session_data.buffet);
  } else {
    printf("You must 'login' first.\n");
  }

  return BANKING_SUCCESS;
}
#endif /* USE_TRANSFER */

/* DRIVER ********************************************************************/

int
main(int argc, char ** argv)
{
  char * in, * args, buffer[MAX_COMMAND_LENGTH];
  command_t cmd; int i, caught_signal = 0;

  /* Input sanitation */
  if (argc != 2) {
    fprintf(stderr, "USAGE: %s port_num\n", argv[0]);
    return EXIT_FAILURE;
  }

  /* Crypto initialization */
  if (init_crypto(old_shmid(&i))) {
    fprintf(stderr, "FATAL: unable to enter secure mode\n");
    return EXIT_FAILURE;
  }

  /* Socket initialization */
  if ((session_data.sock = init_client_socket(argv[1])) < 0) {
    fprintf(stderr, "FATAL: unable to connect to server\n");
    shutdown_crypto(old_shmid(&i));
    return EXIT_FAILURE;
  }

  /* Issue an interactive prompt, terminate only on failure */
  while (!caught_signal && (in = readline(SHELL_PROMPT))) {
    /* Skip prefix whitespace */
    for (i = 0; in[i] == ' '; ++i);
    /* Ignore empty commands */
    if (*(in + i) != '\0') {
      /* Add the original command to the shell history */
      memset(buffer, '\0', MAX_COMMAND_LENGTH);
      strncpy(buffer, in + i, MAX_COMMAND_LENGTH);
      buffer[MAX_COMMAND_LENGTH - 1] = '\0';
      add_history(buffer);
      /* Read in a line, then attempt to associate it with a command */
      if (validate_command(in + i, &cmd, &args)) {
        fprintf(stderr, "ERROR: invalid command '%s'\n", in + i);
      } else {
        /* Set up to signal based on the command's invocation */
        caught_signal = ((cmd == NULL) || cmd(args));
      }
    }
    free(in);
    in = NULL;
  }

  /* Teardown TODO improve on a blank encrypted message */
  if (!session_data.key) { request_key(&session_data.key); }
  clrbuffet(&session_data.buffet);
  encrypt_command(&session_data.buffet, session_data.key);
  put_message(&session_data.buffet, session_data.sock);
  revoke_key(session_data.key);

  destroy_socket(session_data.sock);
  shutdown_crypto(old_shmid(&i));
  putchar('\n');
  return EXIT_SUCCESS;
}

