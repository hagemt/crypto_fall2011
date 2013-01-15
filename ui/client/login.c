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

#ifdef USE_LOGIN
int
login_command(char * args)
{
  size_t i, len;
  char * user, * pin, buffer[MAX_COMMAND_LENGTH];

  /* Input sanitation */
  len = strnlen(args, MAX_COMMAND_LENGTH);
  /* Advance to the first non-space */
  for (i = 0; i < len && *args == ' '; ++i, ++args);
  /* The username is at this position, isolate it */
  for (user = args; i < len && *args != '\0'; ++i, ++args) {
    if (*args == ' ') { *args = '\0'; i = len; }
  }
  #ifndef NDEBUG
  if (*args != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (argument residue)\n", args);
  }
  #endif

  /* Make sure no one is logged in */
  if (session_data.credentials.userlength == 0
   && authenticated(&session_data) == BANKING_SUCCESS) {
    /* Send the message "login [username]" */
    memset(buffer, '\0', MAX_COMMAND_LENGTH);
    snprintf(buffer, MAX_COMMAND_LENGTH, "login %s", user);
    salt_and_pepper(buffer, NULL, &session_data.buffet);
    encrypt_message(&session_data.buffet, session_data.credentials.key);
    send_message(&session_data.buffet, session_data.sock);
    /* Augment the key with bits from the username */
    len = strnlen(user, MAX_COMMAND_LENGTH);
    for (i = 0; i < AUTH_KEY_LENGTH; ++i) {
      session_data.credentials.key[i] ^= user[i % len];
    }
    /* The reply should be reversed, signed with the augmented key */
    recv_message(&session_data.buffet, session_data.sock);
    decrypt_message(&session_data.buffet, session_data.credentials.key);
    for (i = 0; i < MAX_COMMAND_LENGTH; ++i) {
      /* On authentication failure */
      if (session_data.buffet.pbuffer[i] !=
          session_data.buffet.tbuffer[MAX_COMMAND_LENGTH - 1 - i]) {
        /* TODO cleaner execution? */
        fprintf(stderr, "FATAL: BANKING EXPLOIT DETECTED\n");
        clear_buffet(&session_data.buffet);
        /* They're not a bank! Don't send them our PIN! Revoke the key! */
        for (i = 0; i < AUTH_KEY_LENGTH; ++i) {
          session_data.credentials.key[i] ^= user[i % len];
        }
        /* Send a dummy authentication request. TODO spoof PIN? */
        authenticated(&session_data);
        /* Send a dummy authentication request. */
        authenticated(&session_data);
        /* Cause the client to fail TODO more graceful? */
        return BANKING_FAILURE;
      }
    }
    clear_buffet(&session_data.buffet);

    /* Fetch the PIN from the user, and send that next */
    if (fetch_pin(&session_data.terminal_state, &pin) == BANKING_SUCCESS) {
      salt_and_pepper(pin, NULL, &session_data.buffet);
      /* Purge the PIN from memory */
      gcry_create_nonce(pin, strlen(pin));
      gcry_free(pin);
    }
    encrypt_message(&session_data.buffet, session_data.credentials.key);
    send_message(&session_data.buffet, session_data.sock);
    recv_message(&session_data.buffet, session_data.sock);
    decrypt_message(&session_data.buffet, session_data.credentials.key);
    /* If the reply was affirmative */
    if (!strncmp(session_data.buffet.tbuffer,
                 AUTH_LOGIN_MSG, sizeof(AUTH_LOGIN_MSG) - 1)) {
      /* Echo the server's response */
      print_message(&session_data.buffet);
      /* Cache the username in the credentials, login complete */
      memset(session_data.credentials.username, '\0', MAX_COMMAND_LENGTH);
      strncpy(session_data.credentials.username, user, len);
      session_data.credentials.userlength = len;
    }

    /* If we are not authenticated now, there's a problem TODO bomb out? */
    if (authenticated(&session_data) == BANKING_FAILURE) {
      fprintf(stderr, "ERROR: LOGIN AUTHENTICATION FAILURE\n");
      /* Remove the user bits from the key */
      for (i = 0; i < AUTH_KEY_LENGTH; ++i) {
        session_data.credentials.key[i] ^= user[i % len];
      }
    }
  } else {
    printf("You must 'logout' first.\n");
  }

  return BANKING_SUCCESS;
}
#endif /* USE_LOGIN */

