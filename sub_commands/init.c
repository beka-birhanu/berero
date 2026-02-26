#include "init.h"
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#define DIR_PRIVATE_MODE 0700

static int create_dir(const char *path) {
  if (mkdir(path, DIR_PRIVATE_MODE) == -1) {
    if (errno == EEXIST) {
      return SUCCESS;
    } else {
      perror(path);
      return FAILURE;
    }
  }
  return SUCCESS;
}

int init(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  if (create_dir(ENTRY_DIR) == FAILURE)
    return FAILURE;

  if (create_dir(OBJECT_DIR) == FAILURE)
    return FAILURE;

  if (create_dir(BRANCH_DIR) == FAILURE)
    return FAILURE;

  if (create_dir(INDEX_FILE) == FAILURE)
    return FAILURE;

  return SUCCESS;
}
