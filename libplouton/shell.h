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

#ifndef BANKING_SHELL_H
#define BANKING_SHELL_H

#include "types.h"

typedef char *pos_t;

/*! \brief Given a string, finds the command and seperates arguments */
int fetch_command(char *, command_t *, pos_t *);

/*! \brief Given a string, finds the handler and seperates arguments */
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

/*
int quit_command(char *);
int handle_ping_command(handle_arg_t, char *);
*/

#endif /* BANKING_SHELL_H */
