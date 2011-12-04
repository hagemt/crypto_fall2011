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

/* Local includes */
#include "banking_constants.h"
#define USE_LOGIN
#define USE_BALANCE
#define USE_WITHDRAW
#define USE_LOGOUT
#define USE_TRANSFER
#include "banking_commands.h"

#include "crypto_utils.h"
#include "socket_utils.h"

struct client_session_data_t {
  int sock;
  char * user;
  unsigned char * key;
  char pbuffer[MAX_COMMAND_LENGTH];
  unsigned char cbuffer[MAX_COMMAND_LENGTH];
  char tbuffer[MAX_COMMAND_LENGTH];
} session_data;

inline void
reverse_command(char * cmd) {
  size_t i, j;
  for (i = 0, j = MAX_COMMAND_LENGTH - 1; i < j; ++i, --j) {
    cmd[i] ^= cmd[j];
    cmd[j] ^= cmd[i];
    cmd[i] ^= cmd[j];
  }
}

int
authenticated(struct client_session_data_t * session_data)
{
  if (session_data->key) {
    snprintf(session_data->pbuffer, sizeof(AUTH_CHECK_MSG), AUTH_CHECK_MSG);
    encrypt_command(session_data->pbuffer, session_data->key, session_data->cbuffer);
    send(session_data->sock, session_data->cbuffer, MAX_COMMAND_LENGTH, 0);

    recv(session_data->sock, session_data->cbuffer, MAX_COMMAND_LENGTH, 0);
    decrypt_command(session_data->cbuffer, session_data->key, session_data->tbuffer);
    reverse_command(session_data->tbuffer);
    if (!strncmp((char *)session_data->cbuffer, session_data->tbuffer, MAX_COMMAND_LENGTH)) {
      return BANKING_SUCCESS;
    }
  }
  return BANKING_FAILURE;
}

int
login_command(char * cmd)
{
  char * pin, * msg;
  if (authenticated(&session_data)) {
    /* TODO better way to fetch PIN? */
    if ((pin = readline(PIN_PROMPT))) {
      request_key(&session_data.key);
      /* TODO perform actual authorization */
      msg = malloc(2 * MAX_COMMAND_LENGTH);
      snprintf(msg, MAX_COMMAND_LENGTH, "login %s %s", cmd, pin);
      send(session_data.sock, msg, MAX_COMMAND_LENGTH, 0);
      free(pin);
      free(msg);
    }
    if (authenticated(&session_data)) {
      printf("AUTHENTICATION FAILURE\n");
    }
  } else {
    printf("You must 'logout' first.\n");
  }
  return BANKING_SUCCESS;
}

int
balance_command(char * cmd)
{
  size_t len;
  char response_buffer[MAX_COMMAND_LENGTH];
  if (authenticated(&session_data)) {
    len = strnlen(cmd, MAX_COMMAND_LENGTH);
    send(session_data.sock, cmd, len, 0);
    recv(session_data.sock, response_buffer, MAX_COMMAND_LENGTH, 0);
    response_buffer[MAX_COMMAND_LENGTH - 1] = '\0';
    printf("%s\n", response_buffer);
  }
  memset(response_buffer, 0, MAX_COMMAND_LENGTH);
  return BANKING_SUCCESS;
}

int
withdraw_command(char * cmd)
{
  size_t len;
  char response_buffer[MAX_COMMAND_LENGTH];
  if (authenticated(&session_data)) {
    len = strnlen(cmd, MAX_COMMAND_LENGTH);
    send(session_data.sock, cmd, len, 0);
    recv(session_data.sock, response_buffer, MAX_COMMAND_LENGTH, 0);
    response_buffer[MAX_COMMAND_LENGTH - 1] = '\0';
    printf("%s\n", response_buffer);
  }
  memset(response_buffer, 0, MAX_COMMAND_LENGTH);
  return BANKING_SUCCESS;
}

int
logout_command(char * cmd)
{
  if (authenticated(&session_data)) {
    printf("You did not 'login' first.\n");
  } else {
    revoke_key(session_data.key);
    /* TODO proper deauthorization */
    send(session_data.sock, "logout", 7, 0);
    assert(*cmd == '\0');
  }
  /* Ensure we are not authenticated */
  if (!authenticated(&session_data)) {
    fprintf(stderr, "MISAUTHORIZATION DETECTED\n");
    return BANKING_FAILURE;
  }
  return BANKING_SUCCESS;
}

int
transfer_command(char * cmd)
{
  size_t len;
  char response_buffer[MAX_COMMAND_LENGTH];
  if (authenticated(&session_data)) {
    len = strnlen(cmd, MAX_COMMAND_LENGTH);
    send(session_data.sock, cmd, len, 0);
    recv(session_data.sock, response_buffer, MAX_COMMAND_LENGTH, 0);
    response_buffer[MAX_COMMAND_LENGTH - 1] = '\0';
    printf("%s\n", response_buffer);
  }
  memset(response_buffer, 0, MAX_COMMAND_LENGTH);
  return BANKING_SUCCESS;
}

int
main(int argc, char ** argv)
{
  char * in, * args;
  command cmd;
  int caught_signal;

  /* Input sanitation and initialization */
  if (argc != 2) {
    fprintf(stderr, "USAGE: %s port_num\n", argv[0]);
    return EXIT_FAILURE;
  }
  if (init_crypto()) {
    fprintf(stderr, "FATAL: invalid security\n");
    return EXIT_FAILURE;
  }
  if ((session_data.sock = init_client_socket(argv[1])) < 0) {
    fprintf(stderr, "FATAL: proxy unreachable\n");
    return EXIT_FAILURE;
  }

  /* Issue an interactive prompt, terminate only on failure */
  for (caught_signal = 0; !caught_signal && (in = readline(SHELL_PROMPT));) {
    /* Read in a line, then attempt to associate it with a command */
    if (validate(in, &cmd, &args)) {
      /* Ignore empty commands */
      if (*in != '\0') {
        fprintf(stderr, "ERROR: invalid command '%s'\n", in);
      }
    } else {
      /* Add the command to the shell history */
      add_history(in);
      /* Set up to signal based on the command's invocation */
      caught_signal = ((cmd == NULL) || cmd(args));
    }
    /* Cleanup from here down */
    free(in);
    in = NULL;
  }

  destroy_socket(session_data.sock);
  #ifndef NDEBUG
  test_cryptosystem();
  #endif
  shutdown_crypto();
  printf("\n");
  return EXIT_SUCCESS;
}

