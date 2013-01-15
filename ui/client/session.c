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
#include "banking_commands.h"
#include "banking_constants.h"
#include "crypto_utils.h"
#include "socket_utils.h"

/* SESSION DATA **********************************************************/

struct client_session_data_t {
  int sock, shmid, caught_signal;
  struct sigaction signal_action;
  struct credential_t credentials;
  struct buffet_t buffet;
  struct termios terminal_state;
} session_data;

int
authenticated()
{
	int i, status = BANKING_SUCCESS;
	/* Send an authentication verification request */
	salt_and_pepper(BANKING_MSG_AUTH_CHECK, NULL, &session_data.buffet);
	encrypt_message(&session_data.buffet, session_data.credentials.key);
	send_message(&session_data.buffet, session_data.sock);

	/* The response should be the request backwards */
	recv_message(&session_data.buffet, session_data.sock);
	decrypt_message(&session_data.buffet, session_data.credentials.key);
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

/* SESSION DATA **********************************************************/

inline void
do_handshake(struct client_session_data_t *session) {
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
init_prompt(struct client_session_data_t *session)
{
	/* TODO tab-completion for commands */
	rl_bind_key('\t', rl_insert);

	char *input, *args;
	command_t cmd;
	char buffer[MAX_COMMAND_LENGTH];
	int i;
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
        /* rl_ding(); */
      } else {
        /* Set up to signal based on the command's invocation */
        session_data.caught_signal = ((cmd == NULL) || cmd(args));
        /* TODO unnecessary? clear_buffet(&session_data.buffet); */
      }
    }
    free(in);
    in = NULL;
  }
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

void
init_signals(struct client_session_data_t *session)
{
  sigset_t termination_signals;
  struct sigaction old_signal_action;

	/* */
  sigemptyset(&termination_signals);
  memset(&session_data.signal_action, '\0', sizeof(struct sigaction));
  sigfillset(&session_data.signal_action.sa_mask);
  session_data.signal_action.sa_handler = &handle_signal;

	/* Block termination signals */
  sigaction(SIGTERM, NULL, &old_signal_action);
  if (old_signal_action.sa_handler != SIG_IGN) {
    sigaction(SIGTERM, &session_data.signal_action, NULL);
    sigaddset(&termination_signals, SIGTERM);
  }

	/* Block interrupt signals */
  sigaction(SIGINT, NULL, &old_signal_action);
  if (old_signal_action.sa_handler != SIG_IGN) {
    sigaction(SIGINT, &session_data.signal_action, NULL);
    sigaddset(&termination_signals, SIGINT);
  }

	/* */
  memcpy(&session_data.signal_action.sa_mask,
         &termination_signals, sizeof(sigset_t));
  session_data.caught_signal = 0;
}


