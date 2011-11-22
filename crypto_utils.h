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

struct
key_list_t {
  time_t issued, expires;
  unsigned char * key;
  struct key_list_t * next;
} keystore;

int
init_crypto()
{
  /* Reset the keystore */
  keystore.expires = time(&keystore.issued) + AUTH_KEY_TIMEOUT * AUTH_KEY_TIMEOUT;
  keystore.key = malloc(BANKING_KEY_LENGTH * sizeof(unsigned char));
  strncpy((char *)keystore.key, AUTH_CHECK_MSG, sizeof(AUTH_CHECK_MSG));;
  keystore.next = NULL;

  /* Load thread callbacks */
  #ifdef USING_THREADS
  assert(gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread));
  #endif

  /* Verify the correct version has been linked */
  if (!gcry_check_version(GCRYPT_VERSION)) {
    fprintf(stderr, "ERROR: libgcrypt version mismatch\n");
    return BANKING_ERROR;
  }

  /* Set up secure memory pool */
  gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);
  gcry_control(GCRYCTL_INIT_SECMEM, BANKING_SECMEM, 0);
  gcry_control(GCRYCTL_RESUME_SECMEM_WARN);

  /* Finalize gcrypt initialization */
  gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
  if (!gcry_control(GCRYCTL_INITIALIZATION_FINISHED_P)) {
    fprintf(stderr, "ERROR: gcrypt was not properly initialized\n");
    return BANKING_ERROR;
  }
  return BANKING_OK;
}

void
shutdown_crypto()
{
  /* Destroy the keystore */
  struct key_list_t * current, * next;
  keystore.issued = keystore.expires = 0;
  free(keystore.key);
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

int
request_key(unsigned char ** key)
{
  struct key_list_t * current;

  /* Attempt to produce a new key */
  if ((*key = gcry_random_bytes_secure(BANKING_KEY_LENGTH, GCRY_STRONG_RANDOM))) {
    /* Advance to the most recent key */
    current = &keystore;
    while (current->next) {
      current = current->next;
    }

    /* Initialize this key */
    if ((current->next = malloc(sizeof(struct key_list_t)))) {
      current = current->next;
      current->expires = time(&current->issued) + AUTH_KEY_TIMEOUT;
      current->key = *key;
      current->next = NULL;

      return BANKING_OK;
    }
    gcry_free(*key);
    *key = NULL;
  }
  return BANKING_ERROR;
}

int
revoke_key(unsigned char * key)
{
  struct key_list_t * current;

  /* Check if the keystore is valid */
  if (keystore.issued < keystore.expires) {
    /* Check all the keys */
    current = keystore.next;
    while (current) {
      /* Check if this key matches and is still fresh */
      if (strncmp((char *)current->key, (char *)key, BANKING_KEY_LENGTH)) {
        current = current->next;
      } else if (current->issued < current->expires) {
        /* If so, force it to expire */
        current->expires = current->issued;
        gcry_free(current->key);
        current->key = NULL;
        return BANKING_OK;
      }
    }
  }
  return BANKING_ERROR;
}

void
encrypt_command(unsigned char * input, void * key, unsigned char * output)
{
  /* gcry_error_t error_code; */
  gcry_cipher_hd_t handle;

  gcry_cipher_open(&handle, GCRY_CIPHER_SERPENT256, GCRY_CIPHER_MODE_ECB, GCRY_CIPHER_SECURE);
  gcry_cipher_setkey(handle, key, BANKING_KEY_LENGTH);
  /* gcry_cipher_setiv(handle, void * iv, size_t len); */
  gcry_cipher_encrypt(handle, output, MAX_COMMAND_LENGTH, input, MAX_COMMAND_LENGTH);
  gcry_cipher_close(handle);
}

void
decrypt_command(unsigned char * input, void * key, unsigned char * output)
{
  /* gcry_error_t error_code; */
  gcry_cipher_hd_t handle;

  gcry_cipher_open(&handle, GCRY_CIPHER_SERPENT256, GCRY_CIPHER_MODE_ECB, GCRY_CIPHER_SECURE);
  gcry_cipher_setkey(handle, key, BANKING_KEY_LENGTH);
  /* gcry_cipher_setiv(handle, const void *k, size_t l); */
  gcry_cipher_decrypt(handle, output, MAX_COMMAND_LENGTH, input, MAX_COMMAND_LENGTH);
  gcry_cipher_close(handle);
}

inline void
fprintx(FILE * fp, const char * label, unsigned char * c, size_t len)
{
  size_t i;
  fprintf(fp, "%s: ", label);
  for (i = 0; i < len; ++i) {
    fprintf(fp, "%X%X ", (c[i] & 0xF0) >> 4, (c[i] & 0x0F));
  }
  fprintf(fp, "(%u bits)\n", len * 8);
}

void
print_keystore(FILE * fp)
{
  struct key_list_t * current;

  fprintf(fp, "KEYSTORE (SEED: '%s') [TTL: %li] CREATED: %s", keystore.key,
    (long)(keystore.expires - keystore.issued), ctime(&keystore.issued));

  current = &keystore;
  while (current) {
    if (current->key) {
      fprintx(fp, "\tENTRY", current->key, BANKING_KEY_LENGTH);
      fprintf(fp, "\t\tEXPIRES: %s", ctime(&current->expires));
    } else {
      fprintf(fp, "\tENTRY: REVOKED\n");
    }
    fprintf(fp, "\t\tISSUED:  %s", ctime(&current->issued));
    current = current->next;
  }

  fprintf(fp, "\n");
}

int
test_cryptosystem()
{
  unsigned char * m, * c, * p, * k;

  m = calloc(MAX_COMMAND_LENGTH, sizeof(unsigned char));
  c = calloc(MAX_COMMAND_LENGTH, sizeof(unsigned char));
  p = calloc(MAX_COMMAND_LENGTH, sizeof(unsigned char));

  strncpy((char *)m, AUTH_CHECK_MSG, sizeof(AUTH_CHECK_MSG));

  print_keystore(stderr);
  if (request_key(&k)) {
    fprintf(stderr, "ERROR: cannot obtain key\n");
    return BANKING_ERROR;
  }
  print_keystore(stderr);

  encrypt_command(m, k, c);
  decrypt_command(c, k, p);

  fprintf(stderr, "MSG: '%s'\n", m);
  fprintx(stderr, "RAW", m, MAX_COMMAND_LENGTH);
  fprintx(stderr, "KEY", k, BANKING_KEY_LENGTH);
  fprintx(stderr, "ENC", c, MAX_COMMAND_LENGTH);
  fprintf(stderr, "DEC: '%s'\n", p);
  if (strncmp((char *)m, (char *)p, MAX_COMMAND_LENGTH)) {
    fprintf(stderr, "WARNING: plaintext does not match\n");
  }
  fprintf(stderr, "\n");

  print_keystore(stderr);
  if (revoke_key(k)) {
    fprintf(stderr, "WARNING: cannot release key\n");
  }
  print_keystore(stderr);

  free(m);
  free(c);
  free(p);

  return BANKING_OK;
}

#endif