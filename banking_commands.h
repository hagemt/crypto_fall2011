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
  { #CMD_NAME, CMD_NAME ## _command, sizeof(#CMD_NAME) },

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

typedef int (*command)(char *);

struct command_info_t {
  char * name;
  command function;
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
  { "quit", NULL, sizeof("quit") }
};

int
validate_command(char * cmd, command * fun, char ** args)
{
  int invalid;
  size_t i, len;

  len = strnlen(cmd, MAX_COMMAND_LENGTH);
  /* Advance cmd to the first non-space character */
  for (i = 0; *cmd == ' ' && i < len; ++i, ++cmd);
  /* Locate the first blank space after the command */
  for (*args = cmd; **args != '\0' && i < len; ++i, ++*args) {
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

#ifdef HANDLE_LOGIN
int
handle_login_command(char *);
#endif

#ifdef HANDLE_BALANCE
int
handle_balance_command(char *);
#endif

#ifdef HANDLE_WITHDRAW
int
handle_withdraw_command(char *);
#endif

#ifdef HANDLE_LOGOUT
int
handle_logout_command(char *);
#endif

#ifdef HANDLE_TRANSFER
int
handle_transfer_command(char *);
#endif

#ifdef HANDLE_DEPOSIT
int
handle_deposit_command(char *);
#endif

const struct command_info_t handles[] = {
  #ifdef HANDLE_LOGIN
  INIT_COMMAND(handle_login)
  #endif
  #ifdef HANDLE_BALANCE
  INIT_COMMAND(handle_balance)
  #endif
  #ifdef HANDLE_WITHDRAW
  INIT_COMMAND(handle_withdraw)
  #endif
  #ifdef HANDLE_LOGOUT
  INIT_COMMAND(handle_logout)
  #endif
  #ifdef HANDLE_TRANSFER
  INIT_COMMAND(handle_transfer)
  #endif
  #ifdef HANDLE_DEPOSIT
  INIT_COMMAND(handle_deposit)
  #endif
  { "handle_ping", NULL, sizeof("handle_ping") }
};

int
run_handle(char * cmd)
{
  fprintf(stderr, "RUN_HANDLE: '%s'\n", cmd);
  return BANKING_SUCCESS;
}

#endif /* BANKING_COMMANDS_H */
