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

#include "types.h"

int fetch_command(char *, commant_t *, pos_t *);
int fetch_handler(char *, handler_t *, pos_t *);

/* Command stubs */

#ifdef USE_LOGIN
int login_command(char *);
#endif

#ifdef USE_BALANCE
int balance_command(char *);
#endif

#ifdef USE_WITHDRAW
int withdraw_command(char *);
#endif

#ifdef USE_LOGOUT
int logout_command(char *);
#endif

#ifdef USE_TRANSFER
int transfer_command(char *);
#endif

#ifdef USE_DEPOSIT
int deposit_command(char *);
#endif

/* Handler stubs */

#ifdef HANDLE_LOGIN
int handle_login_command(handle_arg_t, char *);
#endif

#ifdef HANDLE_BALANCE
int handle_balance_command(handle_arg_t, char *);
#endif

#ifdef HANDLE_WITHDRAW
int handle_withdraw_command(handle_arg_t, char *);
#endif

#ifdef HANDLE_LOGOUT
int handle_logout_command(handle_arg_t, char *);
#endif

#ifdef HANDLE_TRANSFER
int handle_transfer_command(handle_arg_t, char *);
#endif

#ifdef HANDLE_DEPOSIT
int handle_deposit_command(handle_arg_t, char *);
#endif

/* Generator macros */

#define COMMAND_INFO(CMD_NAME) \
	{ #CMD_NAME, &CMD_NAME##_command, sizeof(#CMD_NAME) },

#define HANDLER_INFO(CMD_NAME) \
	{ #CMD_NAME, &handle_##CMD_NAME##_command, sizeof(#CMD_NAME) },

/* HOWTO: add any strings you'd like to be commands/handlers below
 * TODO: separate these into source, or should configure per-executable? */

const command_info_t commands[] = {
  #ifdef USE_LOGIN
  COMMAND_INFO(login)
  #endif
  #ifdef USE_BALANCE
  COMMAND_INFO(balance)
  #endif
  #ifdef USE_WITHDRAW
  COMMAND_INFO(withdraw)
  #endif
  #ifdef USE_LOGOUT
  COMMAND_INFO(logout)
  #endif
  #ifdef USE_TRANSFER
  COMMAND_INFO(transfer)
  #endif
  #ifdef USE_DEPOSIT
  COMMAND_INFO(deposit)
  #endif
  /* A mandatory command */
  { "quit", NULL, sizeof("quit") }
};

const handler_info_t handlers[] = {
  #ifdef HANDLE_LOGIN
  HANDLER_INFO(login)
  #endif
  #ifdef HANDLE_BALANCE
  HANDLER_INFO(balance)
  #endif
  #ifdef HANDLE_WITHDRAW
  HANDLER_INFO(withdraw)
  #endif
  #ifdef HANDLE_LOGOUT
  HANDLER_INFO(logout)
  #endif
  #ifdef HANDLE_TRANSFER
  HANDLER_INFO(transfer)
  #endif
  #ifdef HANDLE_DEPOSIT
  HANDLER_INFO(deposit)
  #endif
  /* A dummy handler */
  { "ping", NULL, sizeof("ping") }
};

#endif /* BANKING_COMMANDS_H */
