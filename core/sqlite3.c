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
#include <unistd.h>
/* TODO ^ this is only needed for unlink, remove it? */

#include "libplouton.h"

void
destroy_db(const char * db_path, sqlite3 * db_conn)
{
  if (sqlite3_close(db_conn) != SQLITE_OK) {
    fprintf(stderr, "ERROR: unable to close database\n");
  }
  if (db_path && unlink(db_path)) {
    fprintf(stderr, "WARNING: unable to delete database\n");
  }
}

int
init_db(const char * db_path, sqlite3 ** db_conn)
{
  size_t i;
  const char * residue;
  sqlite3_stmt * statement;
  int status, return_status;

  return_status = BANKING_SUCCESS;

  if (sqlite3_open(db_path, db_conn)) {
    fprintf(stderr, "ERROR: unable to open database\n");
    return_status = BANKING_FAILURE;
  }

  #ifdef BANKING_DB_INIT
  /* Create the table with preliminary data */
  if (return_status == BANKING_SUCCESS) {
    status = sqlite3_prepare_v2(*db_conn,
                                SQL_ACCOUNT_CREATE_TABLE,
                                sizeof(SQL_ACCOUNT_CREATE_TABLE),
                                &statement,
                                &residue);
    if (status != SQLITE_OK
        || (status = sqlite3_step(statement)) != SQLITE_DONE) {
      fprintf(stderr,
              "ERROR: unable to create accounts table [code %i]\n",
              status);
      return_status = BANKING_FAILURE;
    }
    status = sqlite3_finalize(statement);
    #ifndef NDEBUG
    if (status != SQLITE_OK) {
      fprintf(stderr,
              "WARNING: unable to finalize create statement [code %i]\n",
              status);
    }
    #endif /* NDEBUG */
  }
  if (return_status == BANKING_SUCCESS) {
    /* Prepare the insert statement */
    status = sqlite3_prepare_v2(*db_conn,
                                SQL_ACCOUNT_INSERT,
                                sizeof(SQL_ACCOUNT_INSERT),
                                &statement,
                                &residue);
    if (status == SQLITE_OK) {
      /* Fill in the statement with each bit of account info */
      for (i = 0; i < sizeof(accounts) /
                      sizeof(struct account_info_t); ++i) {
        /* Individual error status is too granular, indicate generally */
        status = (sqlite3_bind_text(statement, 1,
                                    accounts[i].name.str,
                                    accounts[i].name.len,
                                    SQLITE_STATIC) == SQLITE_OK) &&
                 (sqlite3_bind_text(statement, 2,
                                    accounts[i].pin.str,
                                    accounts[i].pin.len,
                                    SQLITE_STATIC) == SQLITE_OK) &&
                 (sqlite3_bind_int (statement, 3,
                                    accounts[i].balance) == SQLITE_OK) &&
                 (sqlite3_step(statement) == SQLITE_DONE) &&
                 (sqlite3_reset(statement) == SQLITE_OK);
        if (!status) {
          fprintf(stderr,
                  "WARNING: unable to populate account info for '%s'\n",
                  accounts[i].name.str);
        }
      }
    }
    /* Always cleanup the statement */
    status = sqlite3_finalize(statement);
    #ifndef NDEBUG
    if (status != SQLITE_OK) {
      fprintf(stderr,
              "WARNING: unable to finalize insert statement [code %i]\n",
              status);
    }
    #endif /* NDEBUG */
  }
  #endif /* BANKING_DB_INIT */

  #ifndef NDEBUG
  /* Dump the initial contents of the database in debug mode */
  if (return_status == BANKING_SUCCESS) {
    status = sqlite3_prepare_v2(*db_conn,
                                SQL_ACCOUNT_SELECT_ALL,
                                sizeof(SQL_ACCOUNT_SELECT_ALL),
                                &statement,
                                &residue);
    if (status == SQLITE_OK) {
      fprintf(stderr, "INFO:\tName\tPIN\tBalance\t(initial database)\n");
      while (sqlite3_step(statement) == SQLITE_ROW) {
        fprintf(stderr,
                "\t%s\t%s\t%i\n",
                sqlite3_column_text(statement, 0),
                sqlite3_column_text(statement, 1),
                sqlite3_column_int (statement, 2));
      }
    }
    status = sqlite3_finalize(statement);
    if (status != SQLITE_OK) {
      fprintf(stderr,
              "WARNING: unable to finalize select statement [code %i]\n",
              status);
    }
  }
  #endif /* NDEBUG */

  if (return_status != BANKING_SUCCESS) {
    destroy_db(db_path, *db_conn);
    return BANKING_FAILURE;
  }

  return BANKING_SUCCESS;
}

int
do_check(sqlite3 * db_conn, const char ** residue,
         char * name, size_t name_len,
         char * pin,  size_t  pin_len)
{
  sqlite3_stmt * statement;
  int status, return_status;

  return_status = BANKING_SUCCESS;

  /* Setup the system, which ensures cleanup runs */
  status = sqlite3_prepare_v2(db_conn,
                              SQL_ACCOUNT_SELECT_PIN,
                              sizeof(SQL_ACCOUNT_SELECT_PIN),
                              &statement,
                              residue);
  if (status != SQLITE_OK) {
    #ifndef NDEBUG
    fprintf(stderr,
            "ERROR: unable to prepare lookup statement [code %i]\n",
            status);
    #endif
    return_status = BANKING_FAILURE;
  }

  /* Attempt to fill in the name provided */
  if (return_status == BANKING_SUCCESS) {
    status = sqlite3_bind_text(statement, 1, name,
                                             name_len, SQLITE_STATIC);
    if (status != SQLITE_OK) {
      #ifndef NDEBUG
      fprintf(stderr,
              "ERROR: unable to bind lookup parameters [code %i]\n",
              status);
      #endif
      return_status = BANKING_FAILURE;
    }
  }

  /* Provided we have been successful thus far */
  if (return_status == BANKING_SUCCESS) {
    /* Attempt to recieve a row */
    status = sqlite3_step(statement);
    if (status != SQLITE_ROW) {
      #ifndef NDEBUG
      /* If not, non-terminal commands are errors */
      if (status != SQLITE_DONE) {
        fprintf(stderr,
                "ERROR: unable to complete lookup command [code %i]\n",
                status);
      }
      #endif
      /* But either way, the lookup has failed */
      return_status = BANKING_FAILURE;
    } else if (pin) {
      /* The lookup has successfully obtained an entry */
      return_status = strncmp(pin,
                              (char *)(sqlite3_column_text(statement, 0)),
                              pin_len);
      /* The return_status will be non-zero if the pin matches */
    }
  }

  /* Always cleanup the statement */
  status = sqlite3_finalize(statement);
  #ifndef NDEBUG
  if (status != SQLITE_OK) {
    fprintf(stderr,
            "WARNING: unable to finalize update statement [code %i]\n",
            status);
  }
  #endif

  return return_status;
}

int
do_lookup(sqlite3 * db_conn, const char ** residue,
          char * name, size_t name_len, long int * balance)
{
  sqlite3_stmt * statement;
  int status, return_status;

  return_status = BANKING_SUCCESS;

  /* Setup the system, which ensures cleanup runs */
  status = sqlite3_prepare_v2(db_conn,
                              SQL_ACCOUNT_SELECT_BALANCE,
                              sizeof(SQL_ACCOUNT_SELECT_BALANCE),
                              &statement,
                              residue);
  if (status != SQLITE_OK) {
    #ifndef NDEBUG
    fprintf(stderr,
            "ERROR: unable to prepare lookup statement [code %i]\n",
            status);
    #endif
    return_status = BANKING_FAILURE;
  }

  /* Attempt to fill in the name provided */
  if (return_status == BANKING_SUCCESS) {
    status = sqlite3_bind_text(statement, 1, name,
                                             name_len, SQLITE_STATIC);
    if (status != SQLITE_OK) {
      #ifndef NDEBUG
      fprintf(stderr,
              "ERROR: unable to bind lookup parameters [code %i]\n",
              status);
      #endif
      return_status = BANKING_FAILURE;
    }
  }

  /* Provided we have been successful thus far */
  if (return_status == BANKING_SUCCESS) {
    /* Attempt to recieve a row */
    status = sqlite3_step(statement);
    if (status != SQLITE_ROW) {
      #ifndef NDEBUG
      /* If not, non-terminal commands are errors */
      if (status != SQLITE_DONE) {
        fprintf(stderr,
                "ERROR: unable to complete lookup command [code %i]\n",
                status);
      }
      #endif
      /* But either way, the lookup has failed */
      return_status = BANKING_FAILURE;
    } else if (balance) {
      /* The lookup has successfully obtained an entry */
      *balance = sqlite3_column_int(statement, 0);
    }
  }

  /* Always cleanup the statement */
  status = sqlite3_finalize(statement);
  #ifndef NDEBUG
  if (status != SQLITE_OK) {
    fprintf(stderr,
            "WARNING: unable to finalize update statement [code %i]\n",
            status);
  }
  #endif

  return return_status;
}

int
do_update(sqlite3 * db_conn, const char ** residue,
          char * name, size_t name_len, long int new_balance)
{
  sqlite3_stmt * statement;
  int status, return_status;

  /* Checking an update will demand a lookup */
  long int lookup_balance;
  const char * lookup_residue;

  lookup_balance = -1;
  lookup_residue = NULL;
  return_status = BANKING_SUCCESS;

  status = sqlite3_prepare_v2(db_conn,
                              SQL_ACCOUNT_UPDATE_BALANCE,
                              sizeof(SQL_ACCOUNT_UPDATE_BALANCE),
                              &statement,
                              residue);
  if (status != SQLITE_OK) {
    #ifndef NDEBUG
    fprintf(stderr,
            "ERROR: unable to prepare update statement [code %i]\n",
            status);
    #endif
    return_status = BANKING_FAILURE;
  }

  if (return_status == BANKING_SUCCESS
      && (   (status = sqlite3_bind_int (statement, 1,
                                         new_balance)) != SQLITE_OK
          || (status = sqlite3_bind_text(statement, 2,
                                         name, name_len,
                                         SQLITE_STATIC)) != SQLITE_OK)) {
    #ifndef NDEBUG
    fprintf(stderr,
            "ERROR: unable to bind update parameters [code %i]\n",
            status);
    #endif
    return_status = BANKING_FAILURE;
  }

  if (return_status == BANKING_SUCCESS) {
    status = sqlite3_step(statement);
    if (status != SQLITE_DONE) {
      #ifndef NDEBUG
      fprintf(stderr,
              "ERROR: unable to complete update command [code %i]\n",
              status);
      #endif
      return_status = BANKING_FAILURE;
    }
  }

  status = sqlite3_finalize(statement);
  #ifndef NDEBUG
  if (status != SQLITE_OK) {
    fprintf(stderr,
            "WARNING: unable to finalize update statement [code %i]\n",
            status);
  }
  #endif

  #ifndef NDEBUG
  /* Do a quick sanity check */
  if (return_status == BANKING_SUCCESS
      && (   (status = do_lookup(db_conn, &lookup_residue,
                                 name, name_len, &lookup_balance))
          || lookup_balance != new_balance)) {
    fprintf(stderr,
            "WARNING: update failed sanity check [code %i]\n",
            status);
    if (status == SQLITE_OK) {
      fprintf(stderr,
              "WARNING: balance [%li] does not match expected [%li]\n",
              lookup_balance, new_balance);
    }
  }
  if (lookup_residue && *lookup_residue != '\0') {
    fprintf(stderr,
            "WARNING: ignoring extraneous '%s' (query residue)\n",
            lookup_residue);
  }
  #endif /* NDEBUG */

  return return_status;
}
