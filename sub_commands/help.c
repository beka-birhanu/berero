#include "help.h"
#include <stdio.h>

void help(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  printf("Usage:\n");
  printf("  berero init -- Initializes the repository\n");
  printf("  berero help -- Shows this help\n");
}
