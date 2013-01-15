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

#ifdef USE_LOGOUT
int
logout_command(char * args)
{
  int i, len;

  /* Logout command takes no arguments, no input sanitation required */
  #ifndef NDEBUG
  if (*args != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (argument residue)\n", args);
  }
  #endif

  /* Only authenticated users can logout */
  if (session_data.credentials.userlength
   && authenticated(&session_data) == BANKING_SUCCESS) {
    salt_and_pepper("logout", NULL, &session_data.buffet);
    encrypt_message(&session_data.buffet, session_data.credentials.key);
    send_message(&session_data.buffet, session_data.sock);
    recv_message(&session_data.buffet, session_data.sock);
    decrypt_message(&session_data.buffet, session_data.credentials.key);
    print_message(&session_data.buffet);
    /* Ensure we are now not authenticated TODO bomb out? */
    if (authenticated(&session_data) == BANKING_SUCCESS) {
      fprintf(stderr, "ERROR: LOGOUT AUTHENTICATION FAILURE\n");
    }
    /* Remove the user bits from the key */
    len = session_data.credentials.userlength;
    for (i = 0; i < AUTH_KEY_LENGTH; ++i) {
      session_data.credentials.key[i] ^=
       session_data.credentials.username[i % len];
    }
    /* At this point we are sure the user is not authenticated */
    memset(session_data.credentials.username, '\0', MAX_COMMAND_LENGTH);
    session_data.credentials.userlength = 0;
  } else {
    printf("You must 'login' first.\n");
  }
  
  return BANKING_SUCCESS;
}
#endif /* USE_LOGOUT */

