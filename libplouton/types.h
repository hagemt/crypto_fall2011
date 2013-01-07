#ifndef BANKING_TYPES_H
#define BANKING_TYPES_H

#include <stddef.h>

typedef char *str_pos_t;

/*
 * \brief An immutable C-string with attached size (limit)
 */
struct sized_str_t {
	const char *str;
	size_t len;
};

/*
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

/*
 * \brief A command entry (for bank/ATM)
 */
struct command_info_t {
	const struct sized_str_t name;
	command_t function;
};

/*
 * \brief A command-handler entry (for bank/ATM)
 */
struct handler_info_t {
	const struct sized_str_t name;
	handler_t handler;
};

/*
extern const struct account_info_t *accounts;
extern const struct command_info_t *commands;
extern const struct handler_info_t *handlers;
*/

/*! \brief A buffet is an array of buffers, used for encryption/decryption
 *
 *  Inside are the members pbuffer, cbuffer, and tbuffer. For convenience,
 *  cbuffer is an unsigned char[], and the others are char[]s. 
 */
struct buffet_t {
	char pbuffer[BANKING_MAX_COMMAND_LENGTH];
	char tbuffer[BANKING_MAX_COMMAND_LENGTH];
	unsigned char cbuffer[BANKING_MAX_COMMAND_LENGTH];
};

void clear_buffet(struct buffet_t *);

#endif /* BANKING_TYPES_H */
