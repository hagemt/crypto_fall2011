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

#ifdef USE_WITHDRAW
int
withdraw_command(char * args)
{
  size_t i, len;
  long amount;
  char buffer[MAX_COMMAND_LENGTH];

  /* Input sanitation */
  len = strnlen(args, MAX_COMMAND_LENGTH);
  /* Advance to the first non-space */
  for (i = 0; i < len && *args == ' '; ++i, ++args);
  /* TODO check for an acceptable value? */
  amount = strtol(args, &args, 10);
  #ifndef NDEBUG
  if (*args != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (argument residue)\n", args);
  }
  #endif

  /* Only authenticated users may perform withdrawals */
  if (session_data.credentials.userlength
   && authenticated(&session_data) == BANKING_SUCCESS) {
    /* Send the message "withdraw [amount]" */
    memset(buffer, '\0', MAX_COMMAND_LENGTH);
    snprintf(buffer, MAX_COMMAND_LENGTH, "withdraw %li", amount);
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
#endif /* USE_WITHDRAW */
