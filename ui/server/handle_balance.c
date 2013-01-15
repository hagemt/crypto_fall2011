#ifdef HANDLE_BALANCE
int
handle_balance_command(struct thread_data_t * datum, char * args)
{
  long int balance;
  char buffer[MAX_COMMAND_LENGTH];

  /* Balance command takes no arguments */
  #ifndef NDEBUG
  if (*args != '\0') {
    fprintf(stderr,
            "[thread %lu] WARNING: ignoring '%s' (argument residue)\n",
            datum->id, args);
  }
  #endif

  /* Formulate a response */
  memset(buffer, '\0', MAX_COMMAND_LENGTH);
  /* If we have a username, try to do a lookup */
  if (datum->credentials.userlength
   && do_lookup(session_data.db_conn, NULL,
                datum->credentials.username,
                datum->credentials.userlength,
                &balance) == BANKING_SUCCESS) {
    snprintf(buffer, MAX_COMMAND_LENGTH,
             "%s, your balance is $%li.",
             datum->credentials.username, balance);
  } else {
    snprintf(buffer, MAX_COMMAND_LENGTH, "BALANCE ERROR");
  }
  /* Send the results */
  salt_and_pepper(buffer, NULL, &datum->buffet);
  encrypt_message(&datum->buffet, datum->credentials.key);
  send_message(&datum->buffet, datum->sock);
  return BANKING_SUCCESS;
}
#endif /* HANDLE_BALANCE */
