
#ifdef HANDLE_TRANSFER
int
handle_transfer_command(struct thread_data_t * datum, char * args)
{
  long amount, balance;
  char * user, buffer[MAX_COMMAND_LENGTH];

  #ifndef NDEBUG
  if (*args == '\0') {
    fprintf(stderr,
            "[thread %lu] WARNING: [%s] arguments empty\n",
            datum->id, "transfer");
  } else {
    fprintf(stderr,
            "[thread %lu] INFO: [%s] '%s' (arguments)\n",
            datum->id, "transfer", args);
  }
  #endif
  amount = strtol(args, &args, 10);
  user = ++args;

  if (amount <= 0 || amount > MAX_TRANSACTION) {
    snprintf(buffer, MAX_COMMAND_LENGTH, "Invalid transfer amount.");
  } else if (datum->credentials.userlength
   && do_lookup(session_data.db_conn, NULL,
                datum->credentials.username,
                datum->credentials.userlength,
                &balance) == BANKING_SUCCESS) {
    if (balance < amount) {
      snprintf(buffer, MAX_COMMAND_LENGTH, "Insufficient funds.");
    } else if (do_update(session_data.db_conn, NULL,
               datum->credentials.username,
               datum->credentials.userlength,
               balance - amount)
            || do_lookup(session_data.db_conn, NULL,
               user, strnlen(user, MAX_COMMAND_LENGTH),
               &balance)
            || do_update(session_data.db_conn, NULL,
               user, strnlen(user, MAX_COMMAND_LENGTH),
               balance + amount)) {
      /* TODO atomic operation? */
      snprintf(buffer, MAX_COMMAND_LENGTH, "Cannot complete transfer.");
    } else {
      snprintf(buffer, MAX_COMMAND_LENGTH, "Transfered $%li to %s",
                                           amount, user);
    }
  } else {
    snprintf(buffer, MAX_COMMAND_LENGTH, "TRANSFER ERROR");
  }
  salt_and_pepper(buffer, NULL, &datum->buffet);
  encrypt_message(&datum->buffet, datum->credentials.key);
  send_message(&datum->buffet, datum->sock);

  return BANKING_SUCCESS;
}
#endif /* HANDLE_TRANSFER */

