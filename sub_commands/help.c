#include "help.h"
#include <stdio.h>

void help(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  printf("Usage:\n");
  printf("  berero init        -- Initializes the repository\n");
  printf("  berero add <path1 path2 ...>  -- Stages files or directories for tracking\n");
  printf("  berero help        -- Shows this help\n");
}
