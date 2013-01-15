
#ifdef HANDLE_LOGIN
int
handle_login_command(struct thread_data_t * datum, char * args)
{
  size_t i, len;
  char buffer[MAX_COMMAND_LENGTH];

  /* The login argument takes one argument */
  #ifndef NDEBUG
  if (*args == '\0') {
    fprintf(stderr,
            "[thread %lu] WARNING: [%s] arguments empty\n",
            datum->id, "login");
  } else {
    fprintf(stderr,
            "[thread %lu] INFO: [%s] '%s' (arguments)\n",
            datum->id, "login", args);
  }
  #endif
  /* TODO verify they're an actual user of the system */

  /* Modify the key using bits from the username */
  len = strnlen(args, MAX_COMMAND_LENGTH);
  for (i = 0; i < AUTH_KEY_LENGTH; ++i) {
    datum->credentials.key[i] ^= args[i % len];
  }
  /* Turn around the message */
  for (i = 0; i < MAX_COMMAND_LENGTH; ++i) {
    datum->buffet.pbuffer[i] =
     datum->buffet.tbuffer[MAX_COMMAND_LENGTH - 1 - i];
  }
  /* Echo this message with the modified key */
  encrypt_message(&datum->buffet, datum->credentials.key);
  send_message(&datum->buffet, datum->sock);

  /* Receive the PIN */
  recv_message(&datum->buffet, datum->sock);
  decrypt_message(&datum->buffet, datum->credentials.key);
  /* Copy the args backward TODO better auth, use pin as salt */
  memset(buffer, '\0', MAX_COMMAND_LENGTH);
  for (i = 0; i < len; ++i) {
    buffer[i] = args[len - i - 1];
  }

	/* Check the buffer matches the args */
	if (strncmp(buffer, datum->buffet.tbuffer, len)
			|| do_lookup(session_data.db_conn, NULL, args, len, NULL)) {
		snprintf(buffer, MAX_COMMAND_LENGTH, "LOGIN ERROR");
		/* Remove the previously added bits */
		for (i = 0; i < AUTH_KEY_LENGTH; ++i) {
			datum->credentials.key[i] ^= args[i % len];
		}
	} else {
		snprintf(buffer, MAX_COMMAND_LENGTH, "%s, %s!", AUTH_LOGIN_MSG, args);
		/* We have now authenticated the user */
		memset(datum->credentials.username, '\0', MAX_COMMAND_LENGTH);
		strncpy(datum->credentials.username, args, len);
		datum->credentials.userlength = len;
	}
	salt_and_pepper(buffer, NULL, &datum->buffet);
	encrypt_message(&datum->buffet, datum->credentials.key);
	send_message(&datum->buffet, datum->sock);

	/* Catch authentication check */
	recv_message(&datum->buffet, datum->sock);
	decrypt_message(&datum->buffet, datum->credentials.key);
	/* Turn around the message */
	for (i = 0; i < MAX_COMMAND_LENGTH; ++i) {
		datum->buffet.pbuffer[i] =
			datum->buffet.tbuffer[MAX_COMMAND_LENGTH - 1 - i];
	}
	encrypt_message(&datum->buffet, datum->credentials.key);
	send_message(&datum->buffet, datum->sock);

	return BANKING_SUCCESS;
}
#endif /* HANDLE_LOGIN */


