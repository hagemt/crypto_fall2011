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

#ifdef USE_TRANSFER
int
transfer_command(char * args)
{
  size_t i, len;
  long amount;
  char * user, buffer[MAX_COMMAND_LENGTH];

  /* Input sanitation */
  len = strnlen(args, MAX_COMMAND_LENGTH);
  /* Advance to the first non-space */
  for (i = 0; i < len && *args == ' '; ++i, ++args);
  /* Check for an acceptable value TODO check value? */
  amount = strtol(args, &args, 10);
  /* Advance to the next non-space */
  len = strnlen(args, MAX_COMMAND_LENGTH);
  for (i = 0; i < len && *args == ' '; ++i, ++args);
  /* Snag the username and isolate it */
  for (user = args; *args != '\0' && i < len; ++i, ++args) {
    if (*args == ' ') { *args = '\0'; i = len; }
  }
  #ifndef NDEBUG
  if (*args != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (argument residue)\n", args);
  }
  #endif

  /* Only authenticated users may authorize transfers */
  if (session_data.credentials.userlength
   && authenticated(&session_data) == BANKING_SUCCESS) {
    /* Send the command "transfer [amount] [recipient]" */
    memset(buffer, '\0', MAX_COMMAND_LENGTH);
    snprintf(buffer, MAX_COMMAND_LENGTH, "transfer %li %s", amount, user);
    salt_and_pepper(buffer, NULL, &session_data.buffet);
    encrypt_message(&session_data.buffet, session_data.credentials.key);
    send_message(&session_data.buffet, session_data.sock);
    recv_message(&session_data.buffet, session_data.sock);
    decrypt_message(&session_data.buffet, session_data.credentials.key);
    print_message(&session_data.buffet);
  } else {
    printf("You must 'login' first.\n");
  }

  return BANKING_SUCCESS;
}
#endif /* USE_TRANSFER */

