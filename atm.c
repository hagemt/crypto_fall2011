/* Standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Readline includes */
#include <readline/readline.h>

/* Local includes */
#include "banking_constants.h"
#define USE_LOGIN
#define USE_BALANCE
#define USE_WITHDRAW
#define USE_LOGOUT
#define USE_TRANSFER
#include "banking_commands.h"
#include "socket_utils.h"

struct client_session_data_t {
  int sock;
  int key;
  // TODO? char buffer[MAX_COMMAND_LENGTH];
} session_data;

int
authenticated(struct client_session_data_t * session_data) {
  char response_buffer[MAX_COMMAND_LENGTH];
  send(session_data->sock, AUTH_CHECK_MSG, sizeof(AUTH_CHECK_MSG), 0);
  recv(session_data->sock, response_buffer, MAX_COMMAND_LENGTH, 0);
  response_buffer[MAX_COMMAND_LENGTH - 1] = '\0';
  #ifndef NDEBUG
  fprintf(stderr, "INFO: recv '%s'\n", response_buffer);
  #endif
  memset(response_buffer, 0, MAX_COMMAND_LENGTH);
  /* TODO perform actual authentication */
  if (session_data->key) {
    return BANKING_OK;
  }
  return BANKING_ERROR;
}

int
login_command(const char * cmd)
{
  char PIN_buffer[MAX_COMMAND_LENGTH];

  if (authenticated(&session_data)) {
    /* TODO better way to fetch PIN? */
    printf("Enter PIN: ");
    read(0, PIN_buffer, MAX_COMMAND_LENGTH);
    PIN_buffer[MAX_COMMAND_LENGTH - 1] = '\0';

    /* TODO perform actual authorization */
    if (authenticated(&session_data)) {
      printf("AUTHENTICATION FAILURE\n");
    }
  } else {
    printf("You must 'logout' first.\n");
  }

  memset(PIN_buffer, 0, MAX_COMMAND_LENGTH);
  return BANKING_OK;
}

int
balance_command(const char * cmd)
{
  size_t len;
  char response_buffer[MAX_COMMAND_LENGTH];
  if (authenticated(&session_data)) {
    len = strnlen(cmd, MAX_COMMAND_LENGTH);
    send(session_data.sock, cmd, len, 0);
    recv(session_data.sock, response_buffer, MAX_COMMAND_LENGTH, 0);
    response_buffer[MAX_COMMAND_LENGTH - 1] = '\0';
    printf("%s\n", response_buffer);
  }
  memset(response_buffer, 0, MAX_COMMAND_LENGTH);
  return BANKING_OK;
}

int
withdraw_command(const char * cmd)
{
  size_t len;
  char response_buffer[MAX_COMMAND_LENGTH];
  if (authenticated(&session_data)) {
    len = strnlen(cmd, MAX_COMMAND_LENGTH);
    send(session_data.sock, cmd, len, 0);
    recv(session_data.sock, response_buffer, MAX_COMMAND_LENGTH, 0);
    response_buffer[MAX_COMMAND_LENGTH - 1] = '\0';
    printf("%s\n", response_buffer);
  }
  memset(response_buffer, 0, MAX_COMMAND_LENGTH);
  return BANKING_OK;
}

int
logout_command(const char * cmd)
{
  if (authenticated(&session_data)) {
    printf("You did not 'login' first.\n");
  } else {
    /* TODO proper deauthorization */
    session_data.key = 0;
  }
  /* Ensure we are not authenticated */
  if (!authenticated(&session_data)) {
    fprintf(stderr, "MISAUTHORIZATION DETECTED\n");
    return BANKING_ERROR;
  }
  return BANKING_OK;
}

int
transfer_command(const char * cmd)
{
  size_t len;
  char response_buffer[MAX_COMMAND_LENGTH];
  if (authenticated(&session_data)) {
    len = strnlen(cmd, MAX_COMMAND_LENGTH);
    send(session_data.sock, cmd, len, 0);
    recv(session_data.sock, response_buffer, MAX_COMMAND_LENGTH, 0);
    response_buffer[MAX_COMMAND_LENGTH - 1] = '\0';
    printf("%s\n", response_buffer);
  }
  memset(response_buffer, 0, MAX_COMMAND_LENGTH);
  return BANKING_OK;
}

int
main(int argc, char ** argv)
{
  char * in;
  command cmd;
  int caught_signal;

  /* Input sanitation */
  if (argc != 2) {
    fprintf(stderr, "USAGE: %s port_num\n", argv[0]);
    return EXIT_FAILURE;
  }
  if ((session_data.sock = init_client_socket(argv[1])) < 0) {
    fprintf(stderr, "FATAL: proxy unreachable\n");
    return EXIT_FAILURE;
  }

  /* Issue an interactive prompt, terminate only on failure */
  for (caught_signal = 0; !caught_signal && (in = readline(PROMPT));) {
    session_data.key = !session_data.key;
    /* Read in a line, then attempt to associate it with a command */
    if (validate(in, &cmd)) {
      /* Ignore empty commands */
      if (*in != '\0') {
        fprintf(stderr, "ERROR: invalid command '%s'\n", in);
      }
    } else {
      /* Set up to signal based on the command's invocation */
      caught_signal = invoke(in, cmd);
    }
    /* Cleanup from here down */
    free(in);
    in = NULL;
  }

  destroy_socket(session_data.sock);
  printf("\n");
  return EXIT_SUCCESS;
}
