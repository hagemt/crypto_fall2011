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
#include <signal.h>
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

/* SESSION DATA **********************************************************/

struct client_session_data_t {
  int sock, caught_signal;
  struct sigaction signal_action;
  struct credential_t credentials;
  struct buffet_t buffet;
  struct termios terminal_state;
} session_data;

int
authenticated(struct client_session_data_t * session)
{
  int i, status = BANKING_SUCCESS;
  /* Send an authentication verification request */
  salt_and_pepper(AUTH_CHECK_MSG, NULL, &session->buffet);
  encrypt_message(&session->buffet, session->credentials.key);
  send_message(&session->buffet, session->sock);
  /* The response should be the request backwards */
  recv_message(&session->buffet, session->sock);
  decrypt_message(&session->buffet, session->credentials.key);
  for (i = 0; i < MAX_COMMAND_LENGTH; ++i) {
    if (session->buffet.pbuffer[i] !=
        session->buffet.tbuffer[MAX_COMMAND_LENGTH - 1 - i]) {
      status = BANKING_FAILURE;
      break;
    }
  }
  /* Wipe the buffers */
  clear_buffet(&session->buffet);
  return status;
}

int
fetch_pin(struct termios * terminal_state, char ** pin)
{
  /* Fetch the current state of affairs */
  if (tcgetattr(0, terminal_state)) {
    #ifndef NDEBUG
    fprintf(stderr, "ERROR: unable to determine terminal attributes\n");
    #endif
    return BANKING_FAILURE;
  }

  /* Disable terminal echo */
  terminal_state->c_lflag &= ~ECHO;
  if (tcsetattr(0, TCSANOW, terminal_state)) {
    #ifndef NDEBUG
    fprintf(stderr, "ERROR: unable to disable terminal echo\n");
    #endif
    return BANKING_FAILURE;
  }

  /* TODO get readline to use secmem */
  *pin = readline(PIN_PROMPT);
  putchar('\n');

  /* Ensure terminal echo is re-enabled */
  terminal_state->c_lflag |= ECHO;
  if (tcsetattr(0, TCSANOW, terminal_state)) {
    #ifndef NDEBUG
    fprintf(stderr, "ERROR: unable to re-enable terminal echo\n");
    #endif
  }
  
  return (*pin) ? BANKING_SUCCESS : BANKING_FAILURE;
}

inline void
print_message(struct buffet_t * buffet) {
  buffet->tbuffer[MAX_COMMAND_LENGTH - 1] = '\0';
  printf("%s\n", buffet->tbuffer);
  clear_buffet(buffet);
}

/* COMMANDS **************************************************************/

#ifdef USE_LOGIN
int
login_command(char * args)
{
  size_t i, len;
  char * user, * pin, buffer[MAX_COMMAND_LENGTH];

  /* Input sanitation */
  len = strnlen(args, MAX_COMMAND_LENGTH);
  /* Advance to the first non-space */
  for (i = 0; i < len && *args == ' '; ++i, ++args);
  /* The username is at this position, isolate it */
  for (user = args; i < len && *args != '\0'; ++i, ++args) {
    if (*args == ' ') { *args = '\0'; i = len; }
  }
  #ifndef NDEBUG
  if (*args != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (argument residue)\n", args);
  }
  #endif

  /* Make sure no one is logged in */
  if (session_data.credentials.userlength == 0
   && authenticated(&session_data) == BANKING_SUCCESS) {
    /* Send the message "login [username]" */
    memset(buffer, '\0', MAX_COMMAND_LENGTH);
    snprintf(buffer, MAX_COMMAND_LENGTH, "login %s", user);
    salt_and_pepper(buffer, NULL, &session_data.buffet);
    encrypt_message(&session_data.buffet, session_data.credentials.key);
    send_message(&session_data.buffet, session_data.sock);
    /* Augment the key with bits from the username */
    len = strnlen(user, MAX_COMMAND_LENGTH);
    for (i = 0; i < AUTH_KEY_LENGTH; ++i) {
      session_data.credentials.key[i] ^= user[i % len];
    }
    /* The reply should be reversed, signed with the augmented key */
    recv_message(&session_data.buffet, session_data.sock);
    decrypt_message(&session_data.buffet, session_data.credentials.key);
    for (i = 0; i < MAX_COMMAND_LENGTH; ++i) {
      /* On authentication failure */
      if (session_data.buffet.pbuffer[i] !=
          session_data.buffet.tbuffer[MAX_COMMAND_LENGTH - 1 - i]) {
        /* TODO cleaner execution? */
        fprintf(stderr, "FATAL: BANKING EXPLOIT DETECTED\n");
        clear_buffet(&session_data.buffet);
        /* They're not a bank! Don't send them our PIN! Revoke the key! */
        for (i = 0; i < AUTH_KEY_LENGTH; ++i) {
          session_data.credentials.key[i] ^= user[i % len];
        }
        /* Send a dummy authentication request. TODO spoof PIN? */
        authenticated(&session_data);
        /* Send a dummy authentication request. */
        authenticated(&session_data);
        /* Cause the client to fail TODO more graceful? */
        return BANKING_FAILURE;
      }
    }
    clear_buffet(&session_data.buffet);
    
    /* Fetch the PIN from the user, and send that next */
    if (fetch_pin(&session_data.terminal_state, &pin) == BANKING_SUCCESS) {
      salt_and_pepper(pin, NULL, &session_data.buffet);
      /* Purge the PIN from memory */
      gcry_create_nonce(pin, strlen(pin));
      gcry_free(pin);
    }
    encrypt_message(&session_data.buffet, session_data.credentials.key);
    send_message(&session_data.buffet, session_data.sock);
    recv_message(&session_data.buffet, session_data.sock);
    decrypt_message(&session_data.buffet, session_data.credentials.key);
    /* If the reply was affirmative */
    if (!strncmp(session_data.buffet.tbuffer,
                 AUTH_LOGIN_MSG, sizeof(AUTH_LOGIN_MSG) - 1)) {
      /* Echo the server's response */
      print_message(&session_data.buffet);
      /* Cache the username in the credentials, login complete */
      memset(session_data.credentials.username, '\0', MAX_COMMAND_LENGTH);
      strncpy(session_data.credentials.username, user, len);
      session_data.credentials.userlength = len;
    }

    /* If we are not authenticated now, there's a problem TODO bomb out? */
    if (authenticated(&session_data) == BANKING_FAILURE) {
      fprintf(stderr, "ERROR: LOGIN AUTHENTICATION FAILURE\n");
      /* Remove the user bits from the key */
      for (i = 0; i < AUTH_KEY_LENGTH; ++i) {
        session_data.credentials.key[i] ^= user[i % len];
      }
    }
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
  if (session_data.credentials.userlength
   && authenticated(&session_data) == BANKING_SUCCESS) {
    salt_and_pepper("balance", NULL, &session_data.buffet);
    encrypt_message(&session_data.buffet, session_data.credentials.key);
    send_message(&session_data.buffet, session_data.sock);
    recv_message(&session_data.buffet, session_data.sock);
    decrypt_message(&session_data.buffet, session_data.credentials.key);
    print_message(&session_data.buffet);
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
  for (i = 0; i < len && *args == ' '; ++i, ++args);
  /* TODO check for an acceptable value? */
  amount = strtol(args, &args, 10);
  #ifndef NDEBUG
  if (*args != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (argument residue)\n", args);
  }
  #endif

  /* Only authenticated users may perform withdrawals */
  if (session_data.credentials.userlength
   && authenticated(&session_data) == BANKING_SUCCESS) {
    /* Send the message "withdraw [amount]" */
    memset(buffer, '\0', MAX_COMMAND_LENGTH);
    snprintf(buffer, MAX_COMMAND_LENGTH, "withdraw %li", amount);
    salt_and_pepper(buffer, NULL, &session_data.buffet);
    encrypt_message(&session_data.buffet, session_data.credentials.key);
    send_message(&session_data.buffet, session_data.sock);
    recv_message(&session_data.buffet, session_data.sock);
    decrypt_message(&session_data.buffet, session_data.credentials.key);
    print_message(&session_data.buffet);
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
  int i, len;

  /* Logout command takes no arguments, no input sanitation required */
  #ifndef NDEBUG
  if (*args != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (argument residue)\n", args);
  }
  #endif

  /* Only authenticated users can logout */
  if (session_data.credentials.userlength
   && authenticated(&session_data) == BANKING_SUCCESS) {
    salt_and_pepper("logout", NULL, &session_data.buffet);
    encrypt_message(&session_data.buffet, session_data.credentials.key);
    send_message(&session_data.buffet, session_data.sock);
    recv_message(&session_data.buffet, session_data.sock);
    decrypt_message(&session_data.buffet, session_data.credentials.key);
    print_message(&session_data.buffet);
    /* Ensure we are now not authenticated TODO bomb out? */
    if (authenticated(&session_data) == BANKING_SUCCESS) {
      fprintf(stderr, "ERROR: LOGOUT AUTHENTICATION FAILURE\n");
    }
    /* Remove the user bits from the key */
    len = session_data.credentials.userlength;
    for (i = 0; i < AUTH_KEY_LENGTH; ++i) {
      session_data.credentials.key[i] ^=
       session_data.credentials.username[i % len];
    }
    /* At this point we are sure the user is not authenticated */
    memset(session_data.credentials.username, '\0', MAX_COMMAND_LENGTH);
    session_data.credentials.userlength = 0;
  } else {
    printf("You must 'login' first.\n");
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
  char * user, buffer[MAX_COMMAND_LENGTH];

  /* Input sanitation */
  len = strnlen(args, MAX_COMMAND_LENGTH);
  /* Advance to the first non-space */
  for (i = 0; i < len && *args == ' '; ++i, ++args);
  /* Check for an acceptable value TODO check value? */
  amount = strtol(args, &args, 10);
  /* Advance to the next non-space */
  len = strnlen(args, MAX_COMMAND_LENGTH);
  for (i = 0; i < len && *args == ' '; ++i, ++args);
  /* Snag the username and isolate it */
  for (user = args; *args != '\0' && i < len; ++i, ++args) {
    if (*args == ' ') { *args = '\0'; i = len; }
  }
  #ifndef NDEBUG
  if (*args != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (argument residue)\n", args);
  }
  #endif

  /* Only authenticated users may authorize transfers */
  if (session_data.credentials.userlength
   && authenticated(&session_data) == BANKING_SUCCESS) {
    /* Send the command "transfer [amount] [recipient]" */
    memset(buffer, '\0', MAX_COMMAND_LENGTH);
    snprintf(buffer, MAX_COMMAND_LENGTH, "transfer %li %s", amount, user);
    salt_and_pepper(buffer, NULL, &session_data.buffet);
    encrypt_message(&session_data.buffet, session_data.credentials.key);
    send_message(&session_data.buffet, session_data.sock);
    recv_message(&session_data.buffet, session_data.sock);
    decrypt_message(&session_data.buffet, session_data.credentials.key);
    print_message(&session_data.buffet);
  } else {
    printf("You must 'login' first.\n");
  }

  return BANKING_SUCCESS;
}
#endif /* USE_TRANSFER */

/* DRIVER ****************************************************************/

inline void
do_handshake(struct client_session_data_t * session) {
  void * key_addr;
  /* Key initialization TODO error checking? */
  salt_and_pepper(AUTH_CHECK_MSG, NULL, &session->buffet);
  encrypt_message(&session->buffet, keystore.key);
  send_message(&session->buffet, session->sock);
  /* The first message from the server is a session key */
  recv_message(&session->buffet, session->sock);
  decrypt_message(&session->buffet, keystore.key);
  /* Prepare to receive credentials, refer to the message */
  memset(&session->credentials, '\0', sizeof(struct credential_t));
  session->credentials.key =
   (unsigned char *)(key_addr = session->buffet.tbuffer);
  #ifndef NDEBUG
  print_keystore(stderr, "before attach");
  #endif
  /* Attaching the key will copy the space to secmem */
  attach_key(&session->credentials.key);
  #ifndef NDEBUG
  print_keystore(stderr, "after attach");
  #endif
  /* We now have the session key in secmem, clear the message */
  clear_buffet(&session->buffet);
}

void
handle_signal(int signum) {
  int i;
  putchar('\n');
  if (signum) {
    fprintf(stderr,
            "WARNING: signal caught [code %i: %s]\n",
            signum, strsignal(signum));
  }

  /* Disassociate from the server */
  gcry_create_nonce(session_data.buffet.pbuffer, MAX_COMMAND_LENGTH);
  encrypt_message(&session_data.buffet, session_data.credentials.key);
  send_message(&session_data.buffet, session_data.sock);
  recv_message(&session_data.buffet, session_data.sock);
  clear_buffet(&session_data.buffet);
  #ifndef NDEBUG
  print_keystore(stderr, "before revoke");
  #endif
  revoke_key(&session_data.credentials.key);
  #ifndef NDEBUG
  print_keystore(stderr, "after revoke");
  #endif

  /* Ensure terminal echo is re-enabled */
  if (tcgetattr(0, &session_data.terminal_state)) {
    fprintf(stderr, "ERROR: unable to determine terminal attributes\n");
  } else {
    session_data.terminal_state.c_lflag |= ECHO;
    if (tcsetattr(0, TCSANOW, &session_data.terminal_state)) {
      fprintf(stderr, "ERROR: unable to re-enable terminal echo\n");
    }
  }

  /* Shutdown subsystems */
  destroy_socket(session_data.sock);
  shutdown_crypto(old_shmid(&i));

  /* Re-throw termination signals */
  if (sigismember(&session_data.signal_action.sa_mask, signum)) {
    sigemptyset(&session_data.signal_action.sa_mask);
    session_data.signal_action.sa_handler = SIG_DFL;
    sigaction(signum, &session_data.signal_action, NULL);
    raise(signum);
  }
}

int
main(int argc, char ** argv)
{
  char * in, * args, buffer[MAX_COMMAND_LENGTH];
  struct sigaction old_signal_action;
  sigset_t termination_signals;
  command_t cmd;
  int i;

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

  /* Session initialization (setup signals) */
  do_handshake(&session_data);
  sigemptyset(&termination_signals);
  memset(&session_data.signal_action, '\0', sizeof(struct sigaction));
  sigfillset(&session_data.signal_action.sa_mask);
  session_data.signal_action.sa_handler = &handle_signal;
  sigaction(SIGTERM, NULL, &old_signal_action);
  if (old_signal_action.sa_handler != SIG_IGN) {
    sigaction(SIGTERM, &session_data.signal_action, NULL);
    sigaddset(&termination_signals, SIGTERM);
  }
  sigaction(SIGINT, NULL, &old_signal_action);
  if (old_signal_action.sa_handler != SIG_IGN) {
    sigaction(SIGINT, &session_data.signal_action, NULL);
    sigaddset(&termination_signals, SIGINT);
  }
  memcpy(&session_data.signal_action.sa_mask,
         &termination_signals, sizeof(sigset_t));
  session_data.caught_signal = 0;
  /* TODO tab-completion for commands */
  rl_bind_key('\t', rl_insert);

  /* Issue an interactive prompt, terminate only on failure */
  while (!session_data.caught_signal && (in = readline(SHELL_PROMPT))) {
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
      if (validate_command(buffer, &cmd, &args)) {
        fprintf(stderr, "ERROR: invalid command '%s'\n", buffer);
        rl_ding();
      } else {
        /* Set up to signal based on the command's invocation */
        session_data.caught_signal = ((cmd == NULL) || cmd(args));
        /* TODO unnecessary? clear_buffet(&session_data.buffet); */
      }
    }
    free(in);
    in = NULL;
  }

  /* Teardown */
  handle_signal(0);
  return EXIT_SUCCESS;
}

