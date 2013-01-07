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

#include <stdio.h>
#include <stdlib.h>

#define SQUARE(N) ((N) * (N))
#define INVALID_KEY ((unsigned char *)(-1))
#define SHM_RDONLY 0

/* \brief TODO REPLACE
 * If shmid is NULL, do not use shared memory. Otherwise, access it.
 * Put the keystore's seed here if one does not already exist.
 */
int
init_crypto(const int * const shmid)
{
  /* Reset the keystore */
	time(&keystore.issued);
	keystore.expires = keystore.issued + SQUARE(BANKING_AUTH_KEY_TIMEOUT);
	keystore.key = INVALID_KEY;

  /* TODO REMOVE, temporary shared memory solution */
  if (shmid) {
    keystore.key = (unsigned char *) shmat(*shmid, NULL, SHM_RDONLY);
    #ifndef NDEBUG
    fprintf(stderr,
            "INFO: shared memory segment attached at %p\n",
            keystore.key);
    #endif
  }
  if (keystore.key == INVALID_KEY) {
    keystore.key = malloc(BANKING_AUTH_KEY_LENGTH * sizeof(unsigned char));
    strncpy((char *) keystore.key, BANKING_MSG_AUTH_CHECK, sizeof(BANKING_MSG_AUTH_CHECK));
  }
  keystore.next = NULL;

  /* Load thread callbacks */
  #ifdef USING_PTHREADS
  gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
  #endif

  /* Verify the correct version has been linked */
  if (!gcry_check_version(GCRYPT_VERSION)) {
    fprintf(stderr, "ERROR: libgcrypt version mismatch\n");
    return BANKING_FAILURE;
  }

  /* Set up secure memory pool */
  gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);
  gcry_control(GCRYCTL_INIT_SECMEM, BANKING_AUTH_SECMEM, 0);
  gcry_control(GCRYCTL_RESUME_SECMEM_WARN);

  /* Finalize gcrypt initialization */
  gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
  if (!gcry_control(GCRYCTL_INITIALIZATION_FINISHED_P)) {
    fprintf(stderr, "ERROR: gcrypt was not properly initialized\n");
    return BANKING_FAILURE;
  }
  return BANKING_SUCCESS;
}

void
shutdown_crypto(const int * const shmid)
{
  struct key_list_t *current, *next;

  /* Destroy the keystore */
  keystore.issued = keystore.expires = 0;

  /* TODO REMOVE, temporary shared memory solution */
  if (shmid) {
    if (shmdt(keystore.key)) {
      fprintf(stderr, "WARNING: unable to detach shared memory segment\n");
    }
  } else {
    free(keystore.key);
  }

  /* Invalidate all the keys */
  current = keystore.next;
  while (current) {
    current->expires = current->issued;
    if (current->key) {
      gcry_free(current->key);
      current->key = NULL;
    }
    next = current->next;
    free(current);
    current = next;
  }
  keystore.next = NULL;

  /* Destroy secure memory pool */
  gcry_control(GCRYCTL_TERM_SECMEM);
}

/*** ENCRYPTION AND DECRYPTION *******************************************/

/*! \brief Ensure that all buffers are clear */
inline void
clear_buffet(struct buffet_t * buffet) {
	assert(buffet);
	memset(buffet->pbuffer, '\0', BANKING_MAX_COMMAND_LENGTH);
	memset(buffet->cbuffer, '\0', BANKING_MAX_COMMAND_LENGTH);
	memset(buffet->tbuffer, '\0', BANKING_MAX_COMMAND_LENGTH);
}

inline void
encrypt_message(struct buffet_t *buffet, const void *key) {
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

/* We wrap read/write on the socket for simple auditing */

ssize_t
send_message(const struct buffet_t *buffet, int sock) {
	assert(buffet && sock >= 0);
	/* TODO AAA features? */
	return write(sock, buffet->cbuffer, BANKING_MAX_COMMAND_LENGTH);
	/* TODO assurance that only this function can write to sock? */
}

ssize_t
recv_message(const struct buffet_t *buffet, int sock) {
	assert(buffet && sock >= 0);
	/* TODO AAA features? */
	return read(sock, buffet->cbuffer, BANKING_MAX_COMMAND_LENGTH);
	/* TODO assurance that only this function can write to sock? */
}
