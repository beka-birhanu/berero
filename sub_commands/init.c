#include "init.h"
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#define DIR_PRIVATE_MODE 0700

static int create_dir(const char *path) {
  if (mkdir(path, DIR_PRIVATE_MODE) == -1) {
    if (errno == EEXIST) {
      return INIT_OK;
    } else {
      perror(path);
      return INIT_ERROR;
    }
  }
  return INIT_OK;
}

int init(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  if (create_dir(INIT_ENTRY_DIR) == INIT_ERROR)
    return INIT_ERROR;

  if (create_dir(INIT_OBJECT_DIR) == INIT_ERROR)
    return INIT_ERROR;

  if (create_dir(INIT_BRANCH_DIR) == INIT_ERROR)
    return INIT_ERROR;

  if (create_dir(INIT_INDEX_FILE) == INIT_ERROR)
    return INIT_ERROR;

  return INIT_OK;
}
