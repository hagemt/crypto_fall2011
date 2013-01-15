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
#include <stdlib.h>
#include <string.h>

/* TODO static members? */
static struct key_list_t keystore;
#if USING_THREADS
static pthread_mutex_t keystore_mutex;
#endif /* USING_THREADS */

inline int
request_key(key_data_t *key)
{
	/* Key requests are just new attachments */
	if (key) {
		/* TODO better mechanism! (NULLs are unclear) */
		*key = NULL;
	}
	return attach_key(key);
}

int
attach_key(key_data_t *key)
{
	struct key_list_t *entry;
	key_data_t tmp = NULL;

	/* Sanity check the argument */
	if (!key) {
		return BANKING_FAILURE;
	}

	/* Save the address of the current key */
	if (*key) {
		tmp = *key;
	}

	/* Attempt to allocate a new key entry */
	if ((entry = malloc(sizeof(struct key_list_t)))) {
		/* Allocate randomized bytes */
		if ((*key = strong_random_bytes(BANKING_AUTH_KEY_LENGTH))) {
			/* Copy BANKING_AUTH_KEY_LENGTH bytes if available */
			if (tmp) {
				memcpy(*key, tmp, BANKING_AUTH_KEY_LENGTH);
			}
			entry->expires = time(&entry->issued) + BANKING_AUTH_KEY_TIMEOUT;
			entry->key = *key;
			/* Finally, add the completed key entry */
			entry->next = keystore.next;
			keystore.next = entry;
			/* The new key has been inserted (second) */
			return BANKING_SUCCESS;
		}
		/* Otherwise, forget it */
		free(entry);
		*key = tmp;
	}
	return BANKING_FAILURE;
}

int
revoke_key(key_data_t *key)
{
	struct key_list_t *entry;

	/* Sanity check the argument */
	if (!key) {
		return BANKING_FAILURE;
	}

	/* Check if the keystore is valid */
	if (keystore.issued < keystore.expires) {
		entry = keystore.next;
		while (entry) {
			/* Check only valid keys */
			if (entry->issued < entry->expires) {
				/* Check if the key matches */
				if (!strncmp((char *)(entry->key),
				             (char *)(      *key), BANKING_AUTH_KEY_LENGTH)) {
					/* If so, force it to expire and purge */
					entry->expires = entry->issued;
					entry->key = secure_delete(entry->key, BANKING_AUTH_KEY_LENGTH);
					*key = NULL; /* FIXME unnecessary? */
					return BANKING_SUCCESS;
				}
			}
			entry = entry->next;
		}
	}

	return BANKING_FAILURE;
}

#ifdef ENABLE_TESTING
#include <stdio.h>

void
print_keystore(FILE *fp, const char *label)
{
	struct key_list_t *current;

	/* Print the first key in the keystore */
	fprintf(fp, "KEYSTORE (%s) (SEED: '%s') [TTL: %li/%li] CREATED: %s",
			label, keystore.key,
			(long)(keystore.expires - time(NULL)),
			(long)(keystore.expires - keystore.issued),
			ctime(&keystore.issued));

	/* Advance through the keystore, starting at the first subkey */
	for (current = keystore.next; current; current = current->next) {
		if (current->key) {
			fprintx(fp, "\tENTRY", current->key, BANKING_AUTH_KEY_LENGTH);
			fprintf(fp, "\t\tEXPIRES: %s", ctime(&current->expires));
		} else {
			fprintf(fp, "\tENTRY REVOKED\n");
		}
		fprintf(fp, "\t\tISSUED:  %s", ctime(&current->issued));
	}
	fprintf(fp, "\n");
}

#endif /* ENABLE_TESTING */
