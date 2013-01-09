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

#ifndef BANKING_TYPES_H
#define BANKING_TYPES_H

#include <stddef.h>

typedef char *str_pos_t;

/*!
 * \brief An immutable C-string with attached size (limit)
 */
struct sized_str_t {
	const char *str;
	size_t len;
};

/*!
 * \brief An account entry (for the bank DB)
 */
struct account_info_t {
	const struct sized_str_t name, pin;
	long int balance;
};

/* TODO dummy way, bad way? */
typedef struct thread_data_t *handler_arg_t;

typedef int (*command_t)(char *);
typedef int (*handler_t)(handler_arg_t, char *);

/*!
 * \brief A command entry (for bank/ATM)
 */
struct command_info_t {
	const struct sized_str_t name;
	command_t function;
};

/*!
 * \brief A command-handler entry (for bank/ATM)
 */
struct handler_info_t {
	const struct sized_str_t name;
	handler_t handler;
};

extern const struct account_info_t accounts[3];
extern const struct command_info_t commands[];
extern const struct handler_info_t handlers[];

/*!
 * \brief A buffet is an array of buffers, used for encryption/decryption
 *
 * Inside are the members pbuffer, cbuffer, and tbuffer (char[]s)
 * For convenience, cbuffer is actually an unsigned char[]
 */
struct buffet_t {
	char pbuffer[BANKING_MAX_COMMAND_LENGTH];
	char tbuffer[BANKING_MAX_COMMAND_LENGTH];
	unsigned char cbuffer[BANKING_MAX_COMMAND_LENGTH];
};

/*!
 * \brief Reset all data fields in the given buffet
 */
void clear_buffet(struct buffet_t *);

/*!
 * \brief The type information for raw key data.
 */
typedef unsigned char *key_data_t;

#include <time.h>

/*!
 * \brief Essentially a basic (raw) keystore
 *
 * TODO this could be much more sophisticated, for now:
 * It is merely a singly-linked list with timestamps.
 */
struct key_list_t {
	time_t issued, expires;
	key_data_t key;
	struct key_list_t *next;
};

/*
extern const struct key_list_t keystore;
*/

/*!
 * \brief Holds label/key-pairs for users
 */
struct credential_t {
	char buffer[BANKING_MAX_COMMAND_LENGTH];
	size_t position;
	key_data_t key;
};

/*!
 * \brief Specify the label for a credential's key
 *
 * The string given should be of the specified length (checked)
 */
void set_username(struct credential_t *, char *, size_t);

#endif /* BANKING_TYPES_H */
