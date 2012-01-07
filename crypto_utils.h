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

#ifndef CRYPT_UTILS_H
#define CRYPT_UTILS_H

#include <stdio.h>
#include <string.h>
#include <time.h>

#define GCRYPT_NO_DEPRECATED
#ifdef USING_THREADS
#include <assert.h>
#define GCRY_THREAD_OPTION_PTHREAD_IMPL
#endif /* USING_THREADS */
#include <gcrypt.h>

#include "banking_constants.h"

/* SHARED MEMORY TODO REMOVE *************************************************/

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/errno.h>

inline int * new_shmid(int * shmid) {
  char * shmem;
  #ifndef NDEBUG
  fprintf(stderr, "INFO: using shared memory key: 0x%X\n", BANKING_SHMKEY);
  #endif
  *shmid = shmget(BANKING_SHMKEY, sizeof(AUTH_CHECK_MSG), IPC_CREAT | 0666);
  if (*shmid != -1 && (shmem = shmat(*shmid, NULL, 0))) {
    snprintf(shmem, sizeof(AUTH_CHECK_MSG), AUTH_CHECK_MSG);
    #ifndef NDEBUG
    fprintf(stderr, "INFO: shared memory contents: '%s'\n", shmem);
    #endif
  }
  #ifndef NDEBUG
  fprintf(stderr, "INFO: new shared memory id: %i\n", *shmid);
  #endif
  return shmid;
}

inline int * old_shmid(int * shmid) {
  *shmid = shmget(BANKING_SHMKEY, sizeof(AUTH_CHECK_MSG), 0666);
  #ifndef NDEBUG
  fprintf(stderr, "INFO: old shared memory id: %i\n", *shmid);
  #endif
  return shmid;
}

/*** KEYSTORE, ISSUING AND REVOCATION ****************************************/

struct key_list_t {
  time_t issued, expires;
  unsigned char * key;
  struct key_list_t * next;
} keystore;

int
request_key(unsigned char ** key)
{
  struct key_list_t * entry;
  /* TODO remove this */
  *key = keystore.key;
  return BANKING_SUCCESS;

  /* Attempt to produce a new key entry */
  if ((entry = malloc(sizeof(struct key_list_t)))) {
    /* Generate the actual key */
    if ((*key = gcry_random_bytes_secure(AUTH_KEY_LENGTH, GCRY_STRONG_RANDOM))) {
      entry->expires = time(&entry->issued) + AUTH_KEY_TIMEOUT;
      entry->key = *key;
      /* Finally, add the completed key entry */
      entry->next = keystore.next;
      keystore.next = entry;
      return BANKING_SUCCESS;
    }
    /* Otherwise, forget it */
    free(entry);
  }
  return BANKING_FAILURE;
}

int
revoke_key(unsigned char * key)
{
  struct key_list_t * entry;
  /* TODO remove this */
  return BANKING_SUCCESS;

  /* Check if the keystore is valid */
  if (keystore.issued < keystore.expires) {
    entry = keystore.next;
    while (entry) {
      /* Check only valid keys */
      if (entry->issued < entry->expires) {
        /* Check if the key matches */
        if (!strncmp((char *)entry->key, (char *)key, AUTH_KEY_LENGTH)) {
          /* If so, force it to expire and purge */
          entry->expires = entry->issued;
          gcry_create_nonce(entry->key, AUTH_KEY_LENGTH);
          gcry_free(entry->key);
          entry->key = NULL;
          return BANKING_SUCCESS;
        }
      }
      entry = entry->next;
    }
  }
  return BANKING_FAILURE;
}

/*** INITIALIZATION AND TERMINATION ******************************************/

/* \brief TODO remove
 * If shmsp is NULL, do not use shared memory.
 * Otherwise, access it. Put the keystore here if one does not already exist.
 */
int
init_crypto(const int * const shmid)
{
  /* Reset the keystore */
  keystore.expires = time(&keystore.issued) + AUTH_KEY_TIMEOUT * AUTH_KEY_TIMEOUT;
  /* TODO remove temporary shared memory business, do actual OTP */
  keystore.key = (unsigned char *)(-1);
  if (shmid) {
    keystore.key = (unsigned char *)(shmat(*shmid, NULL, SHM_RDONLY));
    #ifndef NDEBUG
    fprintf(stderr, "INFO: shared memory segment attached at %p\n", keystore.key);
    #endif
  }
  if (keystore.key == (unsigned char *)(-1)) {
    keystore.key = malloc(AUTH_KEY_LENGTH * sizeof(unsigned char));
    strncpy((char *)keystore.key, AUTH_CHECK_MSG, sizeof(AUTH_CHECK_MSG));
  }
  keystore.next = NULL;

  /* Load thread callbacks */
  #ifdef USING_THREADS
  assert(gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread));
  #endif

  /* Verify the correct version has been linked */
  if (!gcry_check_version(GCRYPT_VERSION)) {
    fprintf(stderr, "ERROR: libgcrypt version mismatch\n");
    return BANKING_FAILURE;
  }

  /* Set up secure memory pool */
  gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);
  gcry_control(GCRYCTL_INIT_SECMEM, BANKING_SECMEM, 0);
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
  /* Destroy the keystore */
  struct key_list_t * current, * next;
  keystore.issued = keystore.expires = 0;
  /* TODO remove temporary shared memory business */
  if (shmid) {
    if (shmdt(keystore.key)) {
      fprintf(stderr, "WARNING: unable to detach shared memory segment\n");
    }
  } else {
    free(keystore.key);
  }

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

/*** ENCRYPTION AND DECRYPTION ***********************************************/

/*! \brief An array of buffers is a buffet! */
struct buffet_t {
  char pbuffer[MAX_COMMAND_LENGTH], tbuffer[MAX_COMMAND_LENGTH];
  unsigned char cbuffer[MAX_COMMAND_LENGTH];
};

inline char *
strbuffet(struct buffet_t * buffet) {
  buffet->tbuffer[MAX_COMMAND_LENGTH - 1] = '\0';
  return buffet->tbuffer;
}

void
drill_buffers(struct buffet_t * buffet)
{
  int i;
  for (i = 0; i < MAX_COMMAND_LENGTH; ++i) {
    buffet->pbuffer[i] = buffet->tbuffer[MAX_COMMAND_LENGTH - 1 - i];
  }
}

int
check_buffers(struct buffet_t * buffet)
{
  int i, status = BANKING_FAILURE;
  if (buffet) {
    status = BANKING_SUCCESS;
    /* tbuffer should contain pbuffer reversed */
    for (i = 0; i < MAX_COMMAND_LENGTH; ++i) {
      if (buffet->tbuffer[i] != buffet->pbuffer[MAX_COMMAND_LENGTH - 1 - i]) {
        status = BANKING_FAILURE;
        break;
      }
    }
  }
  return status;
}

inline void
clear_buffers(struct buffet_t * buffet) {
  if (buffet) {
    /* TODO noncify instead? */
    memset(buffet->pbuffer, '\0', MAX_COMMAND_LENGTH);
    memset(buffet->cbuffer, '\0', MAX_COMMAND_LENGTH);
    memset(buffet->tbuffer, '\0', MAX_COMMAND_LENGTH);
  }
}

inline void
encrypt_command(struct buffet_t * buffet, void * key) {
  /* gcry_error_t error_code; */
  gcry_cipher_hd_t handle;

  gcry_cipher_open(&handle, GCRY_CIPHER_SERPENT256, GCRY_CIPHER_MODE_ECB, GCRY_CIPHER_SECURE);
  gcry_cipher_setkey(handle, key, AUTH_KEY_LENGTH);

  gcry_cipher_encrypt(handle, buffet->cbuffer, MAX_COMMAND_LENGTH, (unsigned char *)(buffet->pbuffer), MAX_COMMAND_LENGTH);

  gcry_cipher_close(handle);
}

inline void
decrypt_command(struct buffet_t * buffet, void * key) {
  /* gcry_error_t error_code; */
  gcry_cipher_hd_t handle;

  gcry_cipher_open(&handle, GCRY_CIPHER_SERPENT256, GCRY_CIPHER_MODE_ECB, GCRY_CIPHER_SECURE);
  gcry_cipher_setkey(handle, key, AUTH_KEY_LENGTH);

  gcry_cipher_decrypt(handle, (unsigned char *)(buffet->tbuffer), MAX_COMMAND_LENGTH, buffet->cbuffer, MAX_COMMAND_LENGTH);

  gcry_cipher_close(handle);
}

inline ssize_t
put_message(struct buffet_t * buffet, int sock) {
  /* TODO AAA feature */
  return send(sock, buffet->cbuffer, MAX_COMMAND_LENGTH, 0);
}

inline ssize_t
get_message(struct buffet_t * buffet, int sock) {
  /* TODO AAA feature */
  return recv(sock, buffet->cbuffer, MAX_COMMAND_LENGTH, 0);
}

/*** UTILITY FUNCTIONS (INCL. CHECKSUM) **************************************/

void
fprintx(FILE * fp, const char * label, unsigned char * c, size_t len)
{
  size_t i;
  fprintf(fp, "%s: ", label);
  for (i = 0; i < len; ++i) {
    if (i && i * 2 % MAX_COMMAND_LENGTH == 0) { fprintf(stderr, "\n     "); }
    /* TODO remove this line ^ it's only temporary */
    fprintf(fp, "%X%X ", (c[i] & 0xF0) >> 4, (c[i] & 0x0F));
  }
  fprintf(fp, "(%u bits)\n", (unsigned)(len * 8));
}

/*! \brief Obfuscate a message into a buffer, filling the remainder with nonce
 *  
 *  \param msg     A raw message, which will be trimmed to MAX_COMMAND_LENGTH
 *  \param salt    Either NULL or data to XOR with the message
 *  \param buffer  A buffer of size MAX_COMMAND_LENGTH (mandatory)
 */
void
salt_and_pepper(char * msg, const char * salt, struct buffet_t * buffet)
{
  size_t i, mlen, slen;

  /* Buffer the first MAX_COMMAND_LENGTH bytes of the message */
  mlen = strnlen(msg, MAX_COMMAND_LENGTH);
  strncpy(buffet->pbuffer, msg, mlen);
  /* Don't loose the terminator */
  if (mlen < MAX_COMMAND_LENGTH) {
    buffet->pbuffer[mlen] = '\0';
  }

  /* XOR the message with cyclic copies of the salt
   * (for example, msg: "MESSAGE"; salt: "SALT"; yields:
   *  buffer: "MESSAGE" ^ "SALTSAL" + '\0' + nonce)
   */
  if (salt && (slen = strnlen(salt, MAX_COMMAND_LENGTH)) > 0) {
    for (i = 0; i < mlen; ++i) {
      buffet->pbuffer[i] ^= salt[i % slen];
    }
  }

  /* Make sure the message is peppered with nonce */
  if (++mlen < MAX_COMMAND_LENGTH) {
    gcry_create_nonce(buffet->pbuffer + mlen, MAX_COMMAND_LENGTH - mlen);
  }
}

/*! \brief Produce the message digest of a banking command
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
  gcry_md_hash_buffer(GCRY_MD_SHA256, *buffer, (const void *)(cmd), MAX_COMMAND_LENGTH);

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

void
print_keystore(FILE * fp)
{
  struct key_list_t * current;

  fprintf(fp, "KEYSTORE (SEED: '%s') [TTL: %li/%li] CREATED: %s",
    keystore.key,
    (long)(keystore.expires - time(NULL)),
    (long)(keystore.expires - keystore.issued),
    ctime(&keystore.issued));

  current = keystore.next;
  while (current) {
    if (current->key) {
      fprintx(fp, "\tENTRY", current->key, AUTH_KEY_LENGTH);
      fprintf(fp, "\t\tEXPIRES: %s", ctime(&current->expires));
    } else {
      fprintf(fp, "\tENTRY REVOKED\n");
    }
    fprintf(fp, "\t\tISSUED:  %s", ctime(&current->issued));
    current = current->next;
  }

  fprintf(fp, "\n");
}

int
test_crypto()
{
  char * msg;
  struct buffet_t buffet;
  unsigned char * key, * md;

  msg = calloc(MAX_COMMAND_LENGTH, sizeof(char));
  strncpy(msg, AUTH_CHECK_MSG, sizeof(AUTH_CHECK_MSG));

  /* key request test */
  fprintf(stderr, "INTIAL STATE:\n");
  print_keystore(stderr);
  if (request_key(&key)) {
    fprintf(stderr, "ERROR: cannot obtain key\n");
    return BANKING_FAILURE;
  }
  fprintf(stderr, "AFTER REQUEST:\n");
  print_keystore(stderr);

  /* encryption test */
  encrypt_command(&buffet, key);
  decrypt_command(&buffet, key);
  fprintf(stderr, "MSG: '%s'\n", msg);
  fprintx(stderr, "RAW", (unsigned char *)(msg), MAX_COMMAND_LENGTH);
  fprintx(stderr, "KEY", key, AUTH_KEY_LENGTH);
  fprintx(stderr, "ENC", buffet.cbuffer, MAX_COMMAND_LENGTH);
  fprintf(stderr, "DEC: '%s'\n", buffet.tbuffer);
  if (strncmp(msg, buffet.tbuffer, MAX_COMMAND_LENGTH)) {
    fprintf(stderr, "WARNING: plaintext does not match\n");
  }
  fprintf(stderr, "\n");

  /* key revocation test */
  fprintf(stderr, "AFTER ENCRYPTION:\n");
  print_keystore(stderr);
  if (revoke_key(key)) {
    fprintf(stderr, "WARNING: cannot revoke key\n");
  }
  fprintf(stderr, "AFTER REVOCATION:\n");
  print_keystore(stderr);

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
  free(md);

  free(msg);

  return BANKING_SUCCESS;
}

#endif /* CRYPTO_UTILS_H */
