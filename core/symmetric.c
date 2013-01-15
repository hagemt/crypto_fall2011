

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

inline void
clear_buffet(struct buffet_t *buffet)
{
	assert(buffet);
	memset(buffet->pbuffer, '\0', BANKING_MAX_COMMAND_LENGTH);
	memset(buffet->cbuffer, '\0', BANKING_MAX_COMMAND_LENGTH);
	memset(buffet->tbuffer, '\0', BANKING_MAX_COMMAND_LENGTH);
	/* FIXME should noncify instead? */
}

inline void
encrypt_message(struct buffet_t *buffet, const void *key)
{
	/* TODO handle gcry_error_t error_code's? */
	gcry_cipher_hd_t handle;
	assert(buffet && key);
	/*
	 * Use the Serpent syymetric key algorithm (256-bit keys)
	 * In secure-memory mode, with ECB-style blocks
	 */
	gcry_cipher_open(&handle,
			GCRY_CIPHER_SERPENT256,
			GCRY_CIPHER_MODE_ECB,
			GCRY_CIPHER_SECURE);
	/* Load the key provided  and turn over data into plain text */
	gcry_cipher_setkey(handle, key, BANKING_AUTH_KEY_LENGTH);
	gcry_cipher_encrypt(handle,
			buffet->cbuffer, BANKING_MAX_COMMAND_LENGTH,
			(unsigned char *) buffet->pbuffer, BANKING_MAX_COMMAND_LENGTH);
	/* TODO: handle close errors */
	gcry_cipher_close(handle);
}

inline void
decrypt_message(struct buffet_t *buffet, const void *key) {
	/* TODO handle gcry_error_t error_code's? */
	gcry_cipher_hd_t handle;
	assert(buffet && key);
	/* TODO: make this process always match above (inverse) */
	gcry_cipher_open(&handle,
			GCRY_CIPHER_SERPENT256,
			GCRY_CIPHER_MODE_ECB,
			GCRY_CIPHER_SECURE);
	/* Load the key provided and turn over data into ciphertext */
	gcry_cipher_setkey(handle, key, BANKING_AUTH_KEY_LENGTH);
	gcry_cipher_decrypt(handle,
			(unsigned char *) buffet->tbuffer, BANKING_MAX_COMMAND_LENGTH,
			buffet->cbuffer, BANKING_MAX_COMMAND_LENGTH);
	/* TODO: handle close errors */
	gcry_cipher_close(handle);
}

