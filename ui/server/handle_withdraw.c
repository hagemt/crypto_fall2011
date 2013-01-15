#ifdef HANDLE_WITHDRAW
int
handle_withdraw_command(struct thread_data_t * datum, char * args)
{
  long balance, amount;
  char buffer[MAX_COMMAND_LENGTH];

  #ifndef NDEBUG
  if (*args == '\0') {
    fprintf(stderr,
            "[thread %lu] WARNING: [%s] arguments empty\n",
            datum->id, "withdraw");
  } else {
    fprintf(stderr,
            "[thread %lu] INFO: [%s] '%s' (arguments)\n",
            datum->id, "withdraw", args);
  }
  #endif
  amount = strtol(args, &args, 10);

  if (amount <= 0 || amount > MAX_TRANSACTION) {
    snprintf(buffer, MAX_COMMAND_LENGTH, "Invalid withdrawal amount.");
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
               balance - amount)) {
      snprintf(buffer, MAX_COMMAND_LENGTH, "Cannot complete withdrawal.");
    } else {
      snprintf(buffer, MAX_COMMAND_LENGTH, "Withdrew $%li", amount);
    }
  } else {
    snprintf(buffer, MAX_COMMAND_LENGTH, "WITHDRAW ERROR");
  }
  salt_and_pepper(buffer, NULL, &datum->buffet);
  encrypt_message(&datum->buffet, datum->credentials.key);
  send_message(&datum->buffet, datum->sock);

  return BANKING_SUCCESS;
}
#endif /* HANDLE_WITHDRAW */
