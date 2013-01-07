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

void
set_username(struct credential_t *id, char *buffer, size_t len) {
	assert(id && buffer && strnlen(buffer, BANKING_MAX_COMMAND_LENGTH) == len);
	id->userlength = len;
	memset(id->username, '\0', BANKING_MAX_COMMAND_LENGTH);
	strncpy(id->username, buffer, len);
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
		if ((*key = gcry_random_bytes_secure(BANKING_AUTH_KEY_LENGTH, GCRY_STRONG_RANDOM))) {
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
					gcry_create_nonce(entry->key, BANKING_AUTH_KEY_LENGTH);
					gcry_free(entry->key);
					entry->key = *key = NULL;
					return BANKING_SUCCESS;
				}
			}
			entry = entry->next;
		}
	}

	return BANKING_FAILURE;
}
