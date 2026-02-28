#include "init.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#define DIR_PRIVATE_MODE 0700

// Create a directory if it doesn't exist
static inline int create_dir(const char *path) {
  if (!path)
    return INIT_ERROR;
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

// Create a file if it doesn't exist
static inline int create_file(const char *path) {
  if (!path)
    return INIT_ERROR;
  int fd =
      open(path, O_CREAT | O_EXCL | O_WRONLY, 0600); // user-only read/write
  if (fd == -1) {
    if (errno == EEXIST) {
      return INIT_OK;
    } else {
      perror(path);
      return INIT_ERROR;
    }
  }
  close(fd);
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

  if (create_file(INIT_INDEX_FILE) == INIT_ERROR)
    return INIT_ERROR;

  return INIT_OK;
}

int initialized() {
  if (opendir(INIT_ENTRY_DIR) == NULL)
    return 0;

  if (opendir(INIT_OBJECT_DIR) == NULL)
    return 0;

  if (opendir(INIT_BRANCH_DIR) == NULL)
    return 0;

  return 1;
}
