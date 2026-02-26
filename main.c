#include "./sub_commands/help.h"
#include "./sub_commands/init.h"
#include <string.h>

int main(int argc, char *argv[]) {
  if (argc == 1) {
    help(argc, argv);
    return 0;
  }

  if (strcmp(argv[1], HELP_SUBCOMMAND) == 0) {
    help(argc, argv);
    return 0;
  }

  if (strcmp(argv[1], INIT_SUBCOMMAND) == 0) {
    return init(argc, argv);
  }

  return 0;
}
