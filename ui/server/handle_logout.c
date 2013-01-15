
#ifdef HANDLE_LOGOUT
int
handle_logout_command(struct thread_data_t * datum, char * args)
{
  int i, len;
  char buffer[MAX_COMMAND_LENGTH];

  /* Logout command takes no arguments */
  #ifndef NDEBUG
  if (*args != '\0') {
    fprintf(stderr,
            "[thread %lu] WARNING: ignoring '%s' (argument residue)\n",
            datum->id, args);
  }
  #endif

  /* Prepare a reply dependent on our state */
  memset(buffer, '\0', MAX_COMMAND_LENGTH);
  if (datum->credentials.userlength) {
    snprintf(buffer, MAX_COMMAND_LENGTH, "Goodbye, %s!",
             datum->credentials.username);
  } else {
    snprintf(buffer, MAX_COMMAND_LENGTH, "LOGOUT ERROR");
  }
  salt_and_pepper(buffer, NULL, &datum->buffet);
  encrypt_message(&datum->buffet, datum->credentials.key);
  /* Clear the credential bits from the key */
  if (datum->credentials.userlength) {
    len = datum->credentials.userlength;
    for (i = 0; i < AUTH_KEY_LENGTH; ++i) {
      datum->credentials.key[i] ^=
       datum->credentials.username[i % len];
    }
    memset(&datum->credentials.username, '\0', MAX_COMMAND_LENGTH);
    datum->credentials.userlength = 0;
  }
  send_message(&datum->buffet, datum->sock);

  /* Handle verification (should fail) */
  recv_message(&datum->buffet, datum->sock);
  decrypt_message(&datum->buffet, datum->credentials.key);
  for (i = 0; i < MAX_COMMAND_LENGTH; ++i) {
    datum->buffet.pbuffer[i] =
     datum->buffet.tbuffer[MAX_COMMAND_LENGTH - 1 - i];
  }
  encrypt_message(&datum->buffet, datum->credentials.key);
  send_message(&datum->buffet, datum->sock);

  return BANKING_SUCCESS;
}
#endif /* HANDLE_LOGOUT */
