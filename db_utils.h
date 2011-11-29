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

#ifndef DB_UTILS_H
#define DB_UTILS_H

#include <stdio.h>
#include <assert.h>
#include <sqlite3.h>

#include "banking_constants.h"

struct account_info_t
{
  const char * name;
  long int balance;
  size_t length;
};

#define INIT_ACCOUNT(NAME, BALANCE) { #NAME, BALANCE, sizeof(#NAME) }

const struct account_info_t accounts[] = {
  INIT_ACCOUNT(Alice, 100),
  INIT_ACCOUNT(Bob,    50),
  INIT_ACCOUNT(Eve,     0)
};

void
destroy_db(const char * db_path, sqlite3 * db_conn) {
  if (db_path && remove(db_path)) {
    fprintf(stderr, "WARNING: unable to delete database\n");
  }
  if (sqlite3_close(db_conn) != SQLITE_OK) {
    fprintf(stderr, "ERROR: cannot close database\n");
  }
}

int
init_db(const char * db_path, sqlite3 ** db_conn)
{
  size_t i;
  const char * residue;
  sqlite3_stmt * statement;

  if(sqlite3_open(db_path, db_conn)) {
    fprintf(stderr, "ERROR: unable to open database\n");
    destroy_db(db_path, *db_conn);
    return BANKING_ERROR;
  }

  /* Create the table with preliminary data */
  assert(sqlite3_prepare(*db_conn, SQL_CMD_CREATE_TABLE, sizeof(SQL_CMD_CREATE_TABLE), &statement, &residue) == SQLITE_OK);
  assert(sqlite3_step(statement) == SQLITE_DONE);
  assert(sqlite3_finalize(statement) == SQLITE_OK);
  for (i = 0; i < sizeof(accounts) / sizeof(struct account_info_t); ++i) {
    assert(sqlite3_prepare(*db_conn, SQL_CMD_INSERT_ACCOUNT, sizeof(SQL_CMD_INSERT_ACCOUNT), &statement, &residue) == SQLITE_OK);
    assert(sqlite3_bind_text(statement, 1, accounts[i].name, accounts[i].length, SQLITE_STATIC) == SQLITE_OK);
    assert(sqlite3_bind_int(statement, 2, accounts[i].balance) == SQLITE_OK);
    assert(sqlite3_step(statement) == SQLITE_DONE);
    assert(sqlite3_finalize(statement) == SQLITE_OK);
  }

  #ifndef NDEBUG
  assert(sqlite3_prepare(*db_conn, SQL_CMD_SELECT_ALL, sizeof(SQL_CMD_SELECT_ALL), &statement, &residue) == SQLITE_OK);
  fprintf(stderr, "Name\tBalance\n");
  while (sqlite3_step(statement) != SQLITE_DONE) {
    fprintf(stderr, "%s\t%i\n", sqlite3_column_text(statement, 0), sqlite3_column_int(statement, 1));
  }
  assert(sqlite3_finalize(statement) == SQLITE_OK);
  #endif

  return BANKING_OK;
}

int first_row;

int select_callback(void *p_data, int num_fields, char **p_fields, char **p_col_names) {

  int i;

  int* nof_records = (int*) p_data;
  (*nof_records)++;

  if (first_row) {
    first_row = 0;

    for (i=0; i < num_fields; i++) {
      printf("%20s", p_col_names[i]);
    }

    printf("\n");
    for (i=0; i< num_fields*20; i++) {
      printf("=");
    }
    printf("\n");
  }

  for(i=0; i < num_fields; i++) {
    if (p_fields[i]) {
      printf("%20s", p_fields[i]);
    }
    else {
      printf("%20s", " ");
    }
  }

  printf("\n");
  return 0;
}

void select_stmt(sqlite3 * db, const char* stmt) {
  char *errmsg;
  int   ret;
  int   nrecs = 0;

  first_row = 1;

  ret = sqlite3_exec(db, stmt, select_callback, &nrecs, &errmsg);

  if(ret!=SQLITE_OK) {
    printf("Error in select statement %s [%s].\n", stmt, errmsg);
  }
  else {
    printf("\n   %d records returned.\n", nrecs);
  }
}

int
do_lookup(sqlite3 * db_conn, const char ** residue, char * name, size_t name_len, long int * balance)
{
  sqlite3_stmt * statement;
  select_stmt(db_conn, SQL_CMD_SELECT_ALL);
  assert(sqlite3_prepare(db_conn, SQL_CMD_LOOKUP_BALANCE, sizeof(SQL_CMD_LOOKUP_BALANCE), &statement, residue) == SQLITE_OK);
  assert(sqlite3_bind_text(statement, 1, name, name_len, SQLITE_STATIC) == SQLITE_OK);
  assert(sqlite3_step(statement) == SQLITE_DONE);
  *balance = sqlite3_column_int(statement, 0);
  assert(sqlite3_finalize(statement) == SQLITE_OK);
  return BANKING_OK;
}

int
do_update(sqlite3 * db_conn, const char ** residue, char * name, size_t name_len, long int balance)
{
  sqlite3_stmt * statement;
  assert(sqlite3_prepare(db_conn, SQL_CMD_UPDATE_BALANCE, sizeof(SQL_CMD_UPDATE_BALANCE), &statement, residue) == SQLITE_OK);
  assert(sqlite3_bind_int(statement, 1, balance) == SQLITE_OK);
  assert(sqlite3_bind_text(statement, 2, name, name_len, SQLITE_STATIC) == SQLITE_OK);
  assert(sqlite3_step(statement) == SQLITE_DONE);
  /* TODO check for issues with integer overflow, final value? */
  assert(sqlite3_finalize(statement) == SQLITE_OK);
  return BANKING_OK;
}

#endif
