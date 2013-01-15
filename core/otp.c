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

#include <stdio.h>
#include <stdlib.h>

/* TODO new/old shmid shmat/shmdt's the OTP created on launch */

#include <sys/ipc.h>
#include <sys/shm.h>

#include "libplouton.h"

inline int *
new_shmid(int *shmid)
{
	size_t len;
	char *shmem;

#ifndef NDEBUG
	fprintf(stderr, "INFO: using shared memory key: 0x%X\n",
			BANKING_AUTH_SHMKEY);
#endif

	len = sizeof(BANKING_MSG_AUTH_CHECK);
	*shmid = shmget(BANKING_AUTH_SHMKEY, len, IPC_CREAT | 0666);
	/* Try to fill the shared memory segment with OTP */
	if (*shmid != -1 && (shmem = shmat(*shmid, NULL, 0))) {
		snprintf(shmem, len, BANKING_MSG_AUTH_CHECK);
#ifndef NDEBUG
		fprintf(stderr, "INFO: shared memory contents: '%s'\n", shmem);
	} else {
		fprintf(stderr, "INFO: shared memory segment not found!\n");
#endif
	}

#ifndef NDEBUG
	fprintf(stderr, "INFO: new shared memory id: %i\n", *shmid);
#endif

	return shmid;
}

inline int *
old_shmid(int *shmid)
{
	size_t len = sizeof(BANKING_MSG_AUTH_CHECK);
	*shmid = shmget(BANKING_AUTH_SHMKEY, len, IPC_EXCL | 0666);
#ifndef NDEBUG
	fprintf(stderr, "INFO: old shared memory id: %i\n", *shmid);
#endif
	return shmid;
}

#define SQUARE(N) ((N) * (N))
#define INVALID_KEY ((key_data_t) -1)

extern struct key_list_t keystore;

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
    keystore.key = shmat(*shmid, NULL, SHM_RDONLY);
    #ifndef NDEBUG
    fprintf(stderr,
            "INFO: shared memory segment attached at %p\n",
            keystore.key);
    #endif
  }
  if (keystore.key == INVALID_KEY) {
    keystore.key = malloc(sizeof(BANKING_MSG_AUTH_CHECK) * sizeof(char));
    memcpy(keystore.key, BANKING_MSG_AUTH_CHECK, sizeof(BANKING_MSG_AUTH_CHECK));
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
