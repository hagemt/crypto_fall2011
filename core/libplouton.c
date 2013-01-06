#include <assert.h>
#include <string.h>

typedef pos_t char *;

int
fetch_command(char *cmd, command_t *fun, pos_t *args)
{
  size_t i, len;
  int invalid = 0;

  /* Input sanitation */
  len = strnlen(cmd, MAX_COMMAND_LENGTH);
  /* Advance cmd to the first non-space character */
  for (i = 0; i < len && *cmd == ' '; ++i, ++cmd);
  /* Locate the first blank space after the command */
  for (*args = cmd; i < len && **args != '\0'; ++i, ++*args) {
    /* We want to terminate here, the args follow */
    if (**args == ' ') { **args = '\0'; i = len; }
  }

  /* Interate through all known commands with valid functions */
  for (i = 0; commands[i].function;) {
    assert(commands[i].name.length <= MAX_COMMAND_LENGTH);
    invalid = strncmp(commands[i].name, cmd, commands[i].length);
    /* If we find a match, break immediately; otherwise, continue */
    if (invalid) { ++i; } else { break; }
  }
  /* The function is valid, or may be NULL for the final command */
  *fun = commands[i].function;

  /* Non-zero return value iff invalid and not final command */
  return invalid && strncmp(commands[i].name.str, cmd, commands[i].name.len);
}

int
fetch_handle(char *msg, handle_t *hdl, pos_t *args)
{
  int i, len;

	/* Input sanitation */
	len = strnlen(msg, MAX_COMMAND_LENGTH);
	/* Advance to the first non-space */
	for (i = 0; i < len && *msg == ' '; ++i) { ++msg; }
	/* Locate the first blank space after the command */
	for (*args = msg; i < len && **args != '\0'; ++i, ++*args) {
		/* We want to terminate here, the args follow */
		if (**args == ' ') { **args = '\0'; i = len; }
	}

	/* Check each of the handles */
	for (i = 0; handles[i].handle; ++i) {
		/* If the first part of the message has no difference, break */
		if (!strncmp(msg, handles[i].name, handles[i].length)) {
			break;
		}
	}

	/* Non-zero return value if the handle is not valid */
	return ((*hdl = handles[i].handle) == NULL);
}
