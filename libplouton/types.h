#ifndef BANKING_TYPES_H
#define BANKING_TYPES_H

/* TODO dummy way, bad way? */
typedef struct thread_data_t *handler_arg_t;

typedef int (*command_t)(char *);
typedef int (*handler_t)(handler_arg_t, char *);

/*
 * \brief An immutable C-string with attached size (limit)
 */
typedef struct sized_str_t {
	const char *str;
	size_t len;
}

/*
 * \brief An account entry (for the bank DB)
 */
typedef struct account_info_t {
	const sized_str_t name, pin;
	long int balance;
};

/*
 * \brief A command entry (for bank/ATM)
 */
typedef struct command_info_t {
	const size_str_t name;
	command_t function;
};

/*
 * \brief A command-handler entry (for bank/ATM)
 */
typedef struct handler_info_t {
	const sized_str_t name;
	handler_t handler;
};

/*
extern const account_info_t *accounts;
extern const command_info_t *commands;
extern const handler_info_t *handlers;
*/

#endif /* BANKING_TYPES_H */
