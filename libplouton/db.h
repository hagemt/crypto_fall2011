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

#ifndef BANKING_DB_H
#define BANKING_DB_H

#include "sqlite3.h"
/* TODO functionalize, like so:
#ifdef USE_SQLITE3
#include "sqlite3.h"
#endif
*/

/* NOTE: these methods are for sqlite3 ONLY */

/*! \brief Open a connection to the given database. */
int create_db(const char *, sqlite3 **);

/*! \brief Close a connection to a given database. */
void destroy_db(const char *, sqlite3 *);

int do_lookup(
	sqlite3 * db_conn,
	const char ** residue,
	char * name,
	size_t name_len,
	long int * balance
);

int do_update(
	sqlite3 * db_conn,
	const char ** residue,
	char * name,
	size_t name_len,
	long int new_balance
);

int do_check(
	sqlite3 *db_conn,
	const char **residue,
	char * name,
	size_t name_len,
	char * pin,
	size_t pin_len
);

#endif /* BANKING_DB_H */
