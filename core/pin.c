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
#include <readline/readline.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>

int
fetch_pin(struct termios *terminal_state, char **pin)
{
	assert(terminal_state && pin);
	/* Fetch the current state of affairs */
	if (tcgetattr(0, terminal_state)) {
		#ifndef NDEBUG
		fprintf(stderr, "ERROR: unable to determine terminal attributes\n");
		#endif
		return BANKING_FAILURE;
	}

	/* Disable terminal echo */
	terminal_state->c_lflag &= ~ECHO;
	if (tcsetattr(0, TCSANOW, terminal_state)) {
		#ifndef NDEBUG
		fprintf(stderr, "ERROR: unable to disable terminal echo\n");
		#endif
		return BANKING_FAILURE;
	}

	/* TODO get readline to use secmem */
	*pin = readline(BANKING_MSG_PROMPT_PIN);
	putchar('\n');

	/* Ensure terminal echo is re-enabled */
	terminal_state->c_lflag |= ECHO;
	if (tcsetattr(0, TCSANOW, terminal_state)) {
		#ifndef NDEBUG
		fprintf(stderr, "ERROR: unable to re-enable terminal echo\n");
		#endif
		abort();
		/* FIXME should raise/catch signal instead? */
	}

	return (*pin) ? BANKING_SUCCESS : BANKING_FAILURE;
}
