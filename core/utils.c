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

inline void
hexdump(FILE * fp, unsigned char * buffer, size_t len) {
	size_t i, j;
	unsigned int mark;

	/* While there's still buffer, handle lines in sixteen bytes per row */
	for (mark = 0x00, i = 0; i < len; mark += 0x10, i = j) {
		/* First, print index marker */
		fprintf(fp, "[%08X] ", mark);
		for (j = i; j < len && (j - i) < 16; ++j) {
			fprintf(fp, "%X%X ", (buffer[j] & 0xF0) >> 4, (buffer[j] & 0x0F));
		}

		/* Pad with spaces as necessary */
		for (j -= i; j < 16; ++j) {
			fprintf(fp, "   ");
		}

		/* Tack on the ASCII representation, dots for non-printables */
		putc(' ', fp);
		for (j = i; j < len && (j - i) < 16; ++j) {
			putc((buffer[j] < ' ' || buffer[j] > '~') ? '.' : buffer[j], fp);
		}
		putc('\n', fp);
	}
}

/*! \brief Print a labeled hex-formated representation of a string
 *
 *  Format: [label]: XX XX XX XX XX XX XX XX ([len * 8] bits)\n
 */
void
fprintx(FILE * fp, const char * label, unsigned char * c, size_t len)
{
  size_t i;
  fprintf(fp, "%s: ", label);
  for (i = 0; i < len; ++i) {
    /* TODO account for longer lines more neatly: */
    if (i && i * 2 % BANKING_MAX_COMMAND_LENGTH == 0) {
      fprintf(stderr, "\n     ");
    }
    fprintf(fp, "%X%X ", (c[i] & 0xF0) >> 4, (c[i] & 0x0F));
  }
  fprintf(fp, "(%u bits)\n", (unsigned)(len * 8));
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
  gcry_md_hash_buffer(GCRY_MD_SHA256, *buffer,
                                      (const void *)(cmd),
                                      BANKING_MAX_COMMAND_LENGTH);

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

void
print_keystore(FILE * fp, const char * label)
{
	struct key_list_t *current;

	/* Print the first key in the keystore */
	fprintf(fp, "KEYSTORE (%s) (SEED: '%s') [TTL: %li/%li] CREATED: %s",
			label, keystore.key,
			(long)(keystore.expires - time(NULL)),
			(long)(keystore.expires - keystore.issued),
			ctime(&keystore.issued));

	/* Advance through the keystore, starting at the first subkey */
	current = keystore.next;
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
