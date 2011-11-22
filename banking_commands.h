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

#ifdef USE_LOGIN
int
login_command(const char *);
#endif

#ifdef USE_BALANCE
int
balance_command(const char *);
#endif

#ifdef USE_WITHDRAW
int
withdraw_command(const char *);
#endif

#ifdef USE_LOGOUT
int
logout_command(const char *);
#endif

#ifdef USE_TRANSFER
int
transfer_command(const char *);
#endif

#ifdef USE_DEPOSIT
int
deposit_command(const char *);
#endif

typedef int (*command)(const char *);

struct command_t
{
  char * name;
  command function;
  size_t length;
};

#define INIT_COMMAND(CMD_NAME) { #CMD_NAME, CMD_NAME ## _command, sizeof(#CMD_NAME) }

const struct command_t commands[] =
{
  #ifdef USE_LOGIN
  INIT_COMMAND(login),
  #endif
  #ifdef USE_BALANCE
  INIT_COMMAND(balance),
  #endif
  #ifdef USE_WITHDRAW
  INIT_COMMAND(withdraw),
  #endif
  #ifdef USE_LOGOUT
  INIT_COMMAND(logout),
  #endif
  #ifdef USE_TRANSFER
  INIT_COMMAND(transfer),
  #endif
  #ifdef USE_DEPOSIT
  INIT_COMMAND(deposit),
  #endif
  { "quit", NULL, 5 }
};

int
validate(const char * const cmd, command * fun)
{
  int invalid;
  size_t i = 0;
  /* Interate through all commands with valid functions */
  while (commands[i].function) {
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

int
invoke(char * const cmd, command fun)
{
  size_t i;
  char * args = cmd;
  /* Safely probe past spaces */
  for (i = 0; *args != '\0' && i < MAX_COMMAND_LENGTH; ++i, ++args) {
    if (*args == ' ') {
      *args = '\0';
      i = MAX_COMMAND_LENGTH;
    }
  }
  /* Invoke the target command provided it is valid */
  return (fun == NULL) || fun(args);
}

#endif
