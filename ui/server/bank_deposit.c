

#ifdef USE_DEPOSIT
int
deposit_command(char * args)
{
  size_t len, i;
  char * username;
  const char * residue;
  long int amount, balance;

  /* Advance args to the first token, this is the username */
  len = strnlen(args, MAX_COMMAND_LENGTH);
  for (i = 0; *args == ' ' && i < len; ++i, ++args);
  for (username = args; *args != '\0' && i < len; ++i, ++args) {
    if (*args == ' ') { *args = '\0'; i = len; }
  }
  len = strnlen(username, (size_t)(args - username));

  /* We should now be at the addition amount */
  amount = strtol(args, (char **)(&residue), 10);
  #ifndef NDEBUG
  if (*residue != '\0') {
    fprintf(stderr,
            "WARNING: ignoring '%s' (argument residue)\n",
            residue);
  }
  #endif

  /* Ensure the transaction amount is sane, TODO better solution? */
  if (amount < -MAX_TRANSACTION || amount > MAX_TRANSACTION) {
    fprintf(stderr,
            "ERROR: amount (%li) exceeds maximum transaction (%i)\n",
            amount, MAX_TRANSACTION);
    return BANKING_SUCCESS;
  }

  /* Prepare and run actual queries */
  if (do_lookup(session_data.db_conn, &residue,
                username, len, &balance)) {
    fprintf(stderr, "ERROR: no account found for '%s'\n", username);
  } else if (do_update(session_data.db_conn, &residue,
                       username, len, balance + amount)) {
    fprintf(stderr,
            "ERROR: unable to complete request on ('%s', %li)\n",
            username, balance);
  } else {
    printf("A transaction of $%li brings %s's balance from $%li to $%li\n",
           amount, username, balance, balance + amount);
  }
  #ifndef NDEBUG
  if (*residue != '\0') {
    fprintf(stderr, "WARNING: ignoring '%s' (query residue)\n", residue);
  }
  #endif

  return BANKING_SUCCESS;
}
#endif /* USE_DEPOSIT */
