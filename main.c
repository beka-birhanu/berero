#include "./sub_commands/add.h"
#include "./sub_commands/help.h"
#include "./sub_commands/init.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  if (argc == 1) {
    help(argc, argv);
    return EXIT_SUCCESS;
  }

  if (strcmp(argv[1], HELP_SUBCOMMAND) == 0) {
    help(argc - 1, ++argv);
    return EXIT_SUCCESS;
  }

  if (strcmp(argv[1], INIT_SUBCOMMAND) == 0) {
    return init(argc - 1, ++argv);
  }

  if (!initialized()) {
    printf("fatal: not a berero repository: .berero\n");
    help(argc - 1, ++argv);
    return EXIT_FAILURE;
  }

  if (strcmp(argv[1], ADD_SUBCOMMAND) == 0) {
    return add(argc - 1, ++argv);
  }

  return EXIT_SUCCESS;
}
