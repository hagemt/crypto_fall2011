#ifndef DB_UTILS_H
#define DB_UTILS_H

#include <stdio.h>
#include <assert.h>
#include <sqlite3.h>

#include "banking_constants.h"

void
destroy_db(const char * db_path, sqlite3 * db_conn) {
  if (db_path && remove(db_path)) {
    fprintf(stderr, "WARNING: unable to delete database\n");
  }
  if (sqlite3_close(db_conn) != SQLITE_OK) {
    fprintf(stderr, "ERROR: cannot close database\n");
  }
}

#define NUM_INIT_DB_CMDS 5
#define INIT_DB_CMD_LEN 50
const char initial_commands[NUM_INIT_DB_CMDS][INIT_DB_CMD_LEN] = {
  "CREATE TABLE accounts(name, balance INTEGER);",
  "INSERT INTO  accounts values('Alice',   100);",
  "INSERT INTO  accounts values('Bob',      50);",
  "INSERT INTO  accounts values('Eve',       0);",
  "SELECT * FROM accounts;"
};

int
init_db(const char * db_path, sqlite3 * db_conn)
{
  int i; const char * residue;
  sqlite3_stmt * statement;
  if(sqlite3_open(db_path, &db_conn)) {
    fprintf(stderr, "ERROR: unable to open database\n");
    destroy_db(db_path, db_conn);
    return BANKING_ERROR;
  }
  /* Create the table with preliminary data */
  for (i = 0; i < NUM_INIT_DB_CMDS - 1; ++i) {
    sqlite3_prepare(db_conn, initial_commands[i], INIT_DB_CMD_LEN, &statement, &residue);
    assert(sqlite3_step(statement) == SQLITE_DONE);
    assert(sqlite3_reset(statement) == SQLITE_OK);
  }
  #ifndef NDEBUG
  sqlite3_prepare(db_conn, initial_commands[i], INIT_DB_CMD_LEN, &statement, &residue);
  fprintf(stderr, "Name\tBalance\n");
  while (sqlite3_step(statement) != SQLITE_DONE) {
    residue = (const char *)sqlite3_column_text(statement, 0);
    i = sqlite3_column_int(statement, 1);
    fprintf(stderr, "%s\t%i\n", residue, i);
  }
  #endif
  assert(sqlite3_finalize(statement) == SQLITE_OK);
  return BANKING_OK;
}

#endif
