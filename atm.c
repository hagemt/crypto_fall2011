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
#define USE_LOGIN
#define USE_BALANCE
#define USE_WITHDRAW
#define USE_LOGOUT
#define USE_TRANSFER
#include "banking_commands.h"
#include "banking_constants.h"

#include "crypto_utils.h"
#include "socket_utils.h"

/* UTILITIES *****************************************************************/

/*! \brief Obfuscate a message into a buffer, filling the remainder with nonce
 *  
 *  \param msg     A raw message, which will be trimmed to MAX_COMMAND_LENGTH
 *  \param salt    Either NULL or data to XOR with the message
 *  \param buffer  A buffer of size MAX_COMMAND_LENGTH (mandatory)
 */
inline void
salt_and_pepper(char * msg, const char * salt, char * buffer) {
  size_t i, mlen, slen;

  /* Buffer the first MAX_COMMAND_LENGTH bytes of the message */
  mlen = strnlen(msg, MAX_COMMAND_LENGTH);
  strncpy(buffer, msg, mlen);

  /* Salt the message with cyclic copies of the 
   * (ex. salt = "SALT", msg = "MESSAGE", buffer = "MESSAGE" ^ "SALTSAL")
   */
  if (salt) {
    slen = strnlen(salt, MAX_COMMAND_LENGTH);
    for (i = 0; i < mlen; ++i) {
      buffer[i] ^= salt[i % slen];
    }
  }

  /* Make sure the message is peppered with nonce */
  gcry_create_nonce(buffer + mlen, MAX_COMMAND_LENGTH - mlen);
}

/* SESSION DATA **************************************************************/

struct client_session_data_t {
  int sock;
  char * user;
  unsigned char * key;
  char pbuffer[MAX_COMMAND_LENGTH];
  unsigned char cbuffer[MAX_COMMAND_LENGTH];
  char tbuffer[MAX_COMMAND_LENGTH];
} session_data;

inline void
gather_information(struct client_session_data_t * session_data) {
    encrypt_command(session_data->pbuffer, session_data->key, session_data->cbuffer);
    send(session_data->sock, session_data->cbuffer, MAX_COMMAND_LENGTH, 0);
    recv(session_data->sock, session_data->cbuffer, MAX_COMMAND_LENGTH, 0);
    decrypt_command(session_data->cbuffer, session_data->key, session_data->tbuffer);
}

inline void
clear_buffers(struct client_session_data_t * session_data) {
  if (session_data) {
    memset(session_data->pbuffer, 0, MAX_COMMAND_LENGTH);
    memset(session_data->cbuffer, 0, MAX_COMMAND_LENGTH);
    memset(session_data->tbuffer, 0, MAX_COMMAND_LENGTH);
  }
}

int
authenticated(struct client_session_data_t * session_data)
{
  int i, status;
  status = BANKING_FAILURE;
  if (session_data && session_data->key) {
    salt_and_pepper(AUTH_CHECK_MSG, session_data->user, session_data->pbuffer);
    gather_information(session_data);
    /* tbuffer should contain pbuffer reversed */
    status = BANKING_SUCCESS;
    for (i = 0; i < MAX_COMMAND_LENGTH; ++i) {
      if (session_data->tbuffer[i] != session_data->pbuffer[MAX_COMMAND_LENGTH - i - 1]) {
        status = BANKING_FAILURE;
        break;
      }
    }
    clear_buffers(session_data);
  }
  return status;
}

/* Commands ******************************************************************/

int
login_command(char * args)
{
  size_t i, len;
  char * pin;

  len = strnlen(args, MAX_COMMAND_LENGTH);
  /* Advance to the first non-space */
  for (i = 0; *args == ' ' && i < len; ++i, ++args);
  for (session_data.user = args; *args != '\0' && i < len; ++i, ++args) {
    if (*args == ' ') { *args = '\0'; i = len; }
  }
  len = strnlen(args, (size_t)(args - session_data.user));
  #ifndef NDEBUG
  if (*args != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (argument residue)\n", args);
  }
  #endif

  if (authenticated(&session_data)) {
    request_key(&session_data.key);
    salt_and_pepper("login", session_data.user, session_data.pbuffer);
    gather_information(&session_data);
    if ((pin = readline(PIN_PROMPT))) {
      salt_and_pepper(pin, session_data.user, session_data.pbuffer);
      gather_information(&session_data);
      free(pin);
    }
    if (authenticated(&session_data)) {
      revoke_key(session_data.key);
      printf("AUTHENTICATION FAILURE\n");
    }
    clear_buffers(&session_data);
  } else {
    printf("You must 'logout' first.\n");
  }
  return BANKING_SUCCESS;
}

/*! \brief Client side balance command.
 */
int
balance_command(char * args)
{
  #ifndef NDEBUG
  if (*args != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (argument residue)\n", args);
  }
  #endif

  if (authenticated(&session_data)) {
    salt_and_pepper("balance", NULL, session_data.pbuffer);
    gather_information(&session_data);
    session_data.tbuffer[MAX_COMMAND_LENGTH - 1] = '\0';
    printf("%s's balance is: %s\n", session_data.user, session_data.tbuffer);
    clear_buffers(&session_data);
  } else {
    printf("You must 'login' first.\n");
  }
  return BANKING_SUCCESS;
}

int
withdraw_command(char * args)
{
  if (authenticated(&session_data)) {
    printf("You must 'login' first.\n");
  } else {
    printf("Ignoring command 'withdraw %s' (not implemented)\n", args);
  }
  return BANKING_SUCCESS;
}

/*! \brief Client side logout command.
 */
int
logout_command(char * args)
{
  #ifndef NDEBUG
  if (*args != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (argument residue)\n", args);
  }
  #endif

  if (authenticated(&session_data)) {
    printf("You must 'login' first.\n");
  } else {
    salt_and_pepper("logout", session_data.user, session_data.pbuffer);
    gather_information(&session_data);
    revoke_key(session_data.key);
  }
 
 /* Ensure we are now not authenticated */
  if (!authenticated(&session_data)) {
    fprintf(stderr, "MISAUTHORIZATION DETECTED\n");
  }
  clear_buffers(&session_data);
  
  return BANKING_SUCCESS;
}

int
transfer_command(char * args)
{
  if (authenticated(&session_data)) {
    printf("You must 'login' first.\n");
  } else {
    printf("Ignoring command 'withdraw %s' (not implemented)\n", args);
  }
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

