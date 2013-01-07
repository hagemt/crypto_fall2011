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

#ifndef BANKING_KEYSTORE_H
#define BANKING_KEYSTORE_H

#include <time.h>

typedef unsigned char *key_data_t;

struct key_list_t {
	time_t issued, expires;
	key_data_t key;
	struct key_list_t *next;
};

extern struct key_list_t keystore;

#ifdef USING_THREADS
#include <pthread.h>
extern pthread_mutex_t keystore_mutex;
#endif /* USING_THREADS */

int attach_key(key_data_t *);

int request_key(key_data_t *);
int revoke_key(key_data_t *);

#include <stddef.h>

struct credential_t {
	char buffer[BANKING_MAX_COMMAND_LENGTH];
	size_t position;
	key_data_t key;
};

void set_username(struct credential_t *, char *, size_t);

#endif /* BANKING_KEYSTORE_H */
