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

#ifndef BANKING_CRYPTO_H
#define BANKING_CRYPTO_H

extern struct key_list_t keystore;

#ifdef USING_THREADS
#include <pthread.h>
extern pthread_mutex_t keystore_mutex;
#endif /* USING_THREADS */

int attach_key(key_data_t *);

int request_key(key_data_t *);
int revoke_key(key_data_t *);

#include <stddef.h>

void set_username(struct credential_t *, char *, size_t);

/* gcrypt includes */
#define GCRYPT_NO_DEPRECATED
#include <gcrypt.h>
#ifdef USING_PTHREADS
GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif

void *strong_random_bytes(size_t);
void *secure_delete(unsigned char *, size_t);

/* TODO: remove arguments or standardize */
int init_crypto(const int * const);
void shutdown_crypto(const int * const);

/* TODO: (re-)simplify interface? */
void encrypt_message(struct buffet_t * buffet, const void * key);
void decrypt_message(struct buffet_t * buffet, const void * key);

#include <stddef.h>

ssize_t send_message(const struct buffet_t *buffet, int sock);
ssize_t recv_message(const struct buffet_t *buffet, int sock);

#endif /* BANKING_CRYPTO_H */
