/*
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Plouton. If not, see <http://www.gnu.org/licenses/>.
 */

#include "libplouton/signals.h"

static struct sigaction __signal_action;
static sigset_t __termination_signals;
static callback_t __signal_callback;

#include <stddef.h>

static inline void
__die_on(int signum)
{
	struct sigaction old_signal_action;
	sigaction(signum, NULL, &old_signal_action);
	if (old_signal_action.sa_handler != SIG_IGN) {
		sigaction(signum, &__signal_action, NULL);
		sigaddset(&__termination_signals, SIGINT);
	}
}

#include <string.h>

void
init_signals(callback_t callback)
{
	__signal_callback = callback;
	memset(&__signal_action, '\0', sizeof(struct sigaction));
	sigfillset(&__signal_action.sa_mask);
	__signal_action.sa_handler = &handle_signal;
	sigemptyset(&__termination_signals);
	/* Capture SIGINT and SIGTERM */
	__die_on(SIGINT);
	__die_on(SIGTERM);
}

#include <stdio.h>

void
handle_signal(int signum)
{
	/* Report we are going down */
	if (signum) {
		fprintf(stderr,
				"WARNING: stopping service [caught signal %i: %s]\n",
				signum, strsignal(signum));
		/* FIXME strsignal is _GNU_SOURCE? */
	}

	/* Call cleanup routine, if applicable */
	if (__signal_callback) {
		(*__signal_callback)();
	}

	/* Re-raise the proper termination signals */
	if (sigismember(&__termination_signals, signum)) {
		/* Reset the default behavior:
		 * 1) Block no signals while handling this one
		 * 2) Specify the default handler
		 * 3) Associate the handler with this signal
		 * 4) Re-send this signal (default handler will catch)
		 */
		sigemptyset(&__signal_action.sa_mask);
		__signal_action.sa_handler = SIG_DFL;
		sigaction(signum, &__signal_action, NULL);
		raise(signum);
		/* TODO handle errors? how? */
	}
}

