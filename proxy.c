#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char ** argv)
{
  if (argc != 3) {
    fprintf(stderr, "USAGE: %s port_in port_out\n", argv[0]);
  }
  return EXIT_SUCCESS;
}
