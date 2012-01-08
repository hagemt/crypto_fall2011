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

#ifndef BANKING_COMMANDS_H
#define BANKING_COMMANDS_H

#include <assert.h>
#include <string.h>

#include "banking_constants.h"

#define INIT_COMMAND(CMD_NAME) \
  { #CMD_NAME, & CMD_NAME##_command, sizeof(#CMD_NAME) },
#define INIT_HANDLE(CMD_NAME) \
  { #CMD_NAME, & handle_##CMD_NAME##_command, sizeof(#CMD_NAME) },

/* COMMANDS *******************************************************************/

#ifdef USE_LOGIN
int
login_command(char *);
#endif

#ifdef USE_BALANCE
int
balance_command(char *);
#endif

#ifdef USE_WITHDRAW
int
withdraw_command(char *);
#endif

#ifdef USE_LOGOUT
int
logout_command(char *);
#endif

#ifdef USE_TRANSFER
int
transfer_command(char *);
#endif

#ifdef USE_DEPOSIT
int
deposit_command(char *);
#endif

typedef int (*command_t)(char *);

struct command_info_t {
  const char * name;
  command_t function;
  size_t length;
};

const struct command_info_t commands[] = {
  #ifdef USE_LOGIN
  INIT_COMMAND(login)
  #endif
  #ifdef USE_BALANCE
  INIT_COMMAND(balance)
  #endif
  #ifdef USE_WITHDRAW
  INIT_COMMAND(withdraw)
  #endif
  #ifdef USE_LOGOUT
  INIT_COMMAND(logout)
  #endif
  #ifdef USE_TRANSFER
  INIT_COMMAND(transfer)
  #endif
  #ifdef USE_DEPOSIT
  INIT_COMMAND(deposit)
  #endif
  /* A mandatory command */
  { "quit", NULL, sizeof("quit") }
};

int
validate_command(char * cmd, command_t * fun, char ** args)
{
  int invalid;
  size_t i, len;

  /* Input sanitation */
  len = strnlen(cmd, MAX_COMMAND_LENGTH);
  /* Advance cmd to the first non-space character */
  for (i = 0; i < len && *cmd == ' '; ++i, ++cmd);
  /* Locate the first blank space after the command */
  for (*args = cmd; i < len && **args != '\0'; ++i, ++*args) {
    /* We want to terminate here, the args follow */
    if (**args == ' ') { **args = '\0'; i = len; }
  }

  /* Interate through all known commands with valid functions */
  for (i = 0; commands[i].function;) {
    assert(commands[i].length <= MAX_COMMAND_LENGTH);
    /* If we find a match, break immediately; otherwise, continue */
    invalid = strncmp(commands[i].name, cmd, commands[i].length);
    if (invalid) { ++i; } else { break; }
  }
  /* The function is valid, or may be NULL for the final command */
  *fun = commands[i].function;

  /* Non-zero return value iff invalid and not final command */
  return invalid && strncmp(commands[i].name, cmd, commands[i].length);
}

/* HANDLES *******************************************************************/

typedef struct thread_data_t * handle_arg_t;

#ifdef HANDLE_LOGIN
int
handle_login_command(handle_arg_t, char *);
#endif

#ifdef HANDLE_BALANCE
int
handle_balance_command(handle_arg_t, char *);
#endif

#ifdef HANDLE_WITHDRAW
int
handle_withdraw_command(handle_arg_t, char *);
#endif

#ifdef HANDLE_LOGOUT
int
handle_logout_command(handle_arg_t, char *);
#endif

#ifdef HANDLE_TRANSFER
int
handle_transfer_command(handle_arg_t, char *);
#endif

#ifdef HANDLE_DEPOSIT
int
handle_deposit_command(handle_arg_t, char *);
#endif

typedef int (*handle_t)(handle_arg_t, char *);

struct handle_info_t {
  const char * name;
  handle_t handle;
  size_t length;
};

const struct handle_info_t handles[] = {
  #ifdef HANDLE_LOGIN
  INIT_HANDLE(login)
  #endif
  #ifdef HANDLE_BALANCE
  INIT_HANDLE(balance)
  #endif
  #ifdef HANDLE_WITHDRAW
  INIT_HANDLE(withdraw)
  #endif
  #ifdef HANDLE_LOGOUT
  INIT_HANDLE(logout)
  #endif
  #ifdef HANDLE_TRANSFER
  INIT_HANDLE(transfer)
  #endif
  #ifdef HANDLE_DEPOSIT
  INIT_HANDLE(deposit)
  #endif
  /* A dummy handle */
  { "ping", NULL, sizeof("ping") }
};

int
fetch_handle(char * msg, handle_t * hdl, char ** args) {
  int i, len;
  /* Input sanitation */
  len = strnlen(msg, MAX_COMMAND_LENGTH);
  /* Advance to the first non-space */
  for (i = 0; i < len && *msg == ' '; ++i) { ++msg; }
  /* Locate the first blank space after the command */
  for (*args = msg; i < len && **args != '\0'; ++i, ++*args) {
    /* We want to terminate here, the args follow */
    if (**args == ' ') { **args = '\0'; i = len; }
  }
  /* Check each of the handles */
  for (i = 0; handles[i].handle; ++i) {
    /* If the first part of the message has no difference, break */
    if (!strncmp(msg, handles[i].name, handles[i].length)) {
      break;
    }
  }
  /* Non-zero return value if the handle is not valid */
  return ((*hdl = handles[i].handle) == NULL);
}

#endif /* BANKING_COMMANDS_H */
