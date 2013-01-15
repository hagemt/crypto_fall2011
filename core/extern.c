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

#include "libplouton.h"

#define BANKING_NAME(CMD_NAME) \
	{ #CMD_NAME, sizeof(#CMD_NAME) }

#define ACCOUNT_INFO(NAME, PIN, BALANCE) \
	{ BANKING_NAME(NAME), BANKING_NAME(PIN), BALANCE }

const struct account_info_t accounts[] = {
	ACCOUNT_INFO(Alice, BANKING_PIN_ALICE, 100),
	ACCOUNT_INFO(Bob,   BANKING_PIN_BOB,   50),
	ACCOUNT_INFO(Eve,   BANKING_PIN_EVE,   0)
};

/* HOWTO: add any strings you'd like to be commands/handlers below
 * TODO: separate these into source, or should configure per-executable? */

#define COMMAND_INFO(CMD_NAME) \
	{ BANKING_NAME(CMD_NAME), &CMD_NAME##_command },

const struct command_info_t commands[] = {
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
	{ BANKING_NAME(quit), NULL }
};

#define HANDLER_INFO(CMD_NAME) \
	{ BANKING_NAME(CMD_NAME), &handle_##CMD_NAME##_command },

const struct handler_info_t handlers[] = {
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
	{ BANKING_NAME(ping), NULL }
};
