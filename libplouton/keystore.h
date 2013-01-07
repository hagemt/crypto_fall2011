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
	key_data_t *key;
	struct key_list_t *next;
} keystore;

#if USING_THREADS
#include <pthread.h>
const static pthread_mutex_t keystore_mutex;
#endif /* USING_THREADS */

int attach_key(key_data_t *);
int revoke_key(key_data_t *);

inline int
request_key(key_data_t *key) {
	/* Key requests are just new attachments */
	if (key) {
		/* TODO better mechanism! (NULLs are unclear) */
		*key = (key_data_t) 0x0;
	}
	return attach_key(key);
}

#include <stddef.h>

struct credential_t {
	/* TODO use sized string? */
	char username[BANKING_MAX_COMMAND_LENGTH];
	size_t userlength;
	key_data_t key;
};

void set_username(struct credential_t *, char *, size_t);

#endif /* BANKING_KEYSTORE_H */
