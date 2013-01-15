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

/* Note: some routines are defined in
 * audit.c (plaform-specific {send,recv,show}_message})
 * symmetric.c (symmetric-key {encrypt,decrypt}_message)
 * otp.c (1-time-pad-based {init,shutdown}_crypto)
 */

inline void *
random_bytes(size_t len)
{
	return gcry_random_bytes_secure(len, GCRY_STRONG_RANDOM);
}

inline void *
secure_delete(unsigned char *addr, size_t len)
{
	gcry_create_nonce(addr, len);
	gcry_free(addr);
	return NULL;
}

inline void
clear_buffet(struct buffet_t *buffet)
{
	assert(buffet);
	memset(buffet->pbuffer, '\0', BANKING_MAX_COMMAND_LENGTH);
	memset(buffet->cbuffer, '\0', BANKING_MAX_COMMAND_LENGTH);
	memset(buffet->tbuffer, '\0', BANKING_MAX_COMMAND_LENGTH);
}

/*! \brief Obfuscate a message, filling the remainder with nonce
 *
 *  Given: salt_and_pepper("MESSAGE", NULL, &buffet);
 *  Result: buffet.pbuffer = "MESSAGE" + null byte + nonce
 *  (*) If a non-NULL second argument is given, it is XOR'd
 *  \param msg     A raw message (trim to BANKING_MAX_COMMAND_LENGTH)
 *  \param salt    Either NULL or data to XOR with the message
 *  \param buffer  A buffer of size BANKING_MAX_COMMAND_LENGTH (mandatory)
 */
void
salt_and_pepper(const char *msg, const char *salt, struct buffet_t *buffet)
{
	size_t i, msg_len, salt_len;

	/* Buffer the first BANKING_MAX_COMMAND_LENGTH bytes of the message */
	msg_len = strnlen(msg, BANKING_MAX_COMMAND_LENGTH);
	strncpy(buffet->pbuffer, msg, msg_len);
	/* Don't loose the terminator */
	if (msg_len < BANKING_MAX_COMMAND_LENGTH) {
		buffet->pbuffer[msg_len] = '\0';
	}

	/* XOR the message with cyclic copies of the salt
	 * (for example, msg: "MESSAGE"; salt: "SALT"; yields:
	 *  buffer: "MESSAGE" ^ "SALTSAL" + '\0' + nonce)
	 */
	if (salt && (salt_len = strnlen(salt, BANKING_MAX_COMMAND_LENGTH)) > 0) {
		for (i = 0; i < msg_len; ++i) {
			buffet->pbuffer[i] ^= salt[i % salt_len];
		}
	}

	/* Make sure the message is 'peppered' with nonce */
	if (++msg_len < BANKING_MAX_COMMAND_LENGTH) {
		gcry_create_nonce(
				buffet->pbuffer + msg_len,
				BANKING_MAX_COMMAND_LENGTH - msg_len);
	}
	/* FIXME should pepper before salting? */
}

/*! \brief Produce the message digest of a banking command TODO SIMPLIFY
 *
 *  \param cmd    The command to checksum
 *  \param digest If non-NULL, check this against the hash of cmd
 *  \param buffer If NULL, hash to a temporary array
 *                If points to NULL, malloc space for the hash here
 *                Otherwise, buffer is assumed to be appropriate
 *  \return       0 if cmd hashes to digest, non-zero otherwise
 */
int
checksum(char * cmd, unsigned char * digest, unsigned char ** buffer)
{
  unsigned char * temporary;
  int status;
  size_t len;

  temporary = NULL;
  status = BANKING_SUCCESS;
  len = gcry_md_get_algo_dlen(GCRY_MD_SHA256);

  /* Behavior is dependent upon the status of arguments
   * If buffer is NULL, cmd is hashed to a temporary array
   * If buffer points to a NULL, allocate space for the hash here
   * Otherwise, buffer is assumed to provide a reasonable buffer
   */
  if (buffer == NULL) {
    temporary = malloc(len);
    buffer = &temporary;
  } else if (*buffer == NULL) {
    *buffer = malloc(len);
  }

  /* Try to take the hash of the command and buffer it */
  gcry_md_hash_buffer(GCRY_MD_SHA256,
			*buffer, (const void *)(cmd), BANKING_MAX_COMMAND_LENGTH);

  /* If a digest value was given, check it */
  if (digest) {
    status = strncmp((char *)(*buffer), (char *)(digest), len);
    #ifndef NDEBUG
    fprintf(stderr, "INFO: sha256sum('%s')", cmd);
    fprintx(stderr, " ?", digest, len);
    if (status) {
      fprintf(stderr, "ERROR: hash mismatch (%i) ", status);
      fprintx(stderr, "HASH =", *buffer, len);
    }
  } else {
    fprintf(stderr, "INFO: sha256sum('%s')", cmd);
    fprintx(stderr, " =", *buffer, len);
    #endif
  }

  /* If a temporary buffer was allocated, release it */
  if (temporary) {
    free(temporary);
  }
  return status;
}

#ifdef ENABLE_TESTING
int
test_crypto()
{
	char * msg;
	struct buffet_t buffet;
	unsigned char * key, * md;

	msg = calloc(BANKING_MAX_COMMAND_LENGTH, sizeof(char));
	strncpy(msg, BANKING_MSG_AUTH_CHECK, sizeof(BANKING_MSG_AUTH_CHECK));

	/* key request test */
	print_keystore(stderr, "initial test state");
	if (request_key(&key)) {
		fprintf(stderr, "ERROR: cannot obtain test key\n");
		return BANKING_FAILURE;
	}
	print_keystore(stderr, "after test key request");

	/* encryption test */
	encrypt_message(&buffet, key);
	decrypt_message(&buffet, key);
	fprintf(stderr, "MSG: '%s'\n", msg);
	fprintx(stderr, "RAW", (unsigned char *)(msg), BANKING_MAX_COMMAND_LENGTH);
	fprintx(stderr, "KEY", key, BANKING_AUTH_KEY_LENGTH);
	fprintx(stderr, "ENC", buffet.cbuffer, BANKING_MAX_COMMAND_LENGTH);
	fprintf(stderr, "DEC: '%s'\n", buffet.tbuffer);
	if (strncmp(msg, buffet.tbuffer, BANKING_MAX_COMMAND_LENGTH)) {
		fprintf(stderr, "WARNING: plaintext does not match\n");
	}
	fputc('\n', stderr);

	/* key revocation test */
	print_keystore(stderr, "after test encryption");
	if (revoke_key(&key)) {
		fprintf(stderr, "WARNING: cannot revoke test key\n");
	}
	print_keystore(stderr, "after test key revocation");

	/* checksum test */
	md = NULL;
	if (checksum(msg, NULL, &md)) {
		fprintf(stderr, "ERROR: cannot checksum message\n");
	}
	if (checksum(msg, md, NULL)) {
		fprintf(stderr, "ERROR: cannot verify message checksum\n");
	} else {
		fprintf(stderr, "INFO: checksum verified\n");
	}

	/* cleanup */
	free(md);
	free(msg);

	return BANKING_SUCCESS;
}
#endif /* ENABLE_TESTING */
