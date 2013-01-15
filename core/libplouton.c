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

#include <assert.h>
#include <string.h>

int
fetch_command(char *cmd, command_t *fun, pos_t *args)
{
	size_t i, len;
	int invalid = 0;

	/* Input sanitation */
	len = strnlen(cmd, BANKING_MAX_COMMAND_LENGTH);
	/* Advance cmd to the first non-space character */
	for (i = 0; i < len && *cmd == ' '; ++i, ++cmd);
	/* Locate the first blank space after the command */
	for (*args = cmd; i < len && **args != '\0'; ++i, ++*args) {
		/* We want to terminate here, the args follow */
		if (**args == ' ') { **args = '\0'; i = len; }
	}

	/* Interate through all known commands with valid functions */
	for (i = 0; commands[i].function;) {
		assert(commands[i].name.len <= BANKING_MAX_COMMAND_LENGTH);
		invalid = strncmp(commands[i].name.str, cmd, commands[i].name.len);
		/* If we find a match, break immediately; otherwise, continue */
		if (invalid) { ++i; } else { break; }
	}
	/* The function is valid, or may be NULL for the final command */
	*fun = commands[i].function;

	/* Non-zero return value iff invalid and not final command */
	return invalid && strncmp(commands[i].name.str, cmd, commands[i].name.len);
}

int
fetch_handle(char *msg, handler_t *hdl, pos_t *args)
{
	int i, len;

	/* Input sanitation */
	len = strnlen(msg, BANKING_MAX_COMMAND_LENGTH);
	/* Advance to the first non-space */
	for (i = 0; i < len && *msg == ' '; ++i) { ++msg; }
	/* Locate the first blank space after the command */
	for (*args = msg; i < len && **args != '\0'; ++i, ++*args) {
		/* We want to terminate here, the args follow */
		if (**args == ' ') { **args = '\0'; i = len; }
	}

	/* Check each of the handles */
	for (i = 0; handlers[i].handler; ++i) {
		/* If the first part of the message has no difference, break */
		if (!strncmp(msg, handlers[i].name.str, handlers[i].name.len)) {
			break;
		}
	}

	/* Non-zero return value if the handle is not valid */
	return ((*hdl = handlers[i].handler) == NULL);
}
