#define _XOPEN_SOURCE 700
#include "linked_list.h"
#include <errno.h>
#include <ftw.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_OPEN_FILES 20

typedef struct {
  struct LinkedList *list;
  int error;
} WalkContext;

static WalkContext *ctx = NULL;

static int callback(const char *fpath, const struct stat *sb, int typeflag,
                    struct FTW *ftwbuf) {
  (void)ftwbuf; // unused
  (void)sb;     // unused

  if (!ctx || ctx->error)
    return 1; // stop on error

  // Only regular files
  if (typeflag == FTW_F) {
    if (ll_append(ctx->list, fpath, (void *)1) != LL_OK) {
      ctx->error = 1;
      return 1; // stop
    }
  }

  return 0; // continue
}

struct LinkedList *walk(const char *path) {
  if (!path)
    return NULL;

  struct LinkedList *list = ll_new();
  if (!list)
    return NULL;

  struct stat st;
  if (stat(path, &st) == -1) {
    perror(path);
    ll_free(list);
    return NULL;
  }

  // Single file case
  if (S_ISREG(st.st_mode)) {
    char *key = strdup(path);
    if (!key || ll_append(list, key, (void *)1) != LL_OK) {
      free(key);
      ll_free(list);
      return NULL;
    }
    return list;
  }

  WalkContext context = {
      .list = list,
      .error = 0,
  };
  ctx = &context;

  int flags = FTW_PHYS;
  if (nftw(path, callback, MAX_OPEN_FILES, flags) == -1) {
    perror("nftw");
    ll_free(list);
    ctx = NULL;
    return NULL;
  }

  ctx = NULL;

  if (context.error) {
    ll_free(list);
    return NULL;
  }

  return list;
}

int is_path_in_cwd(const char *path) {
  if (!path)
    return 0;

  // Get current working directory
  char cwd[PATH_MAX];
  if (!getcwd(cwd, sizeof(cwd))) {
    perror("getcwd");
    return 0;
  }

  // Resolve the absolute path of the file
  char *abs_path = realpath(path, NULL);
  if (!abs_path) {
    perror(path);
    return 0;
  }

  size_t cwd_len = strlen(cwd);

  // Check if abs_path starts with cwd
  int inside = strncmp(abs_path, cwd, cwd_len) == 0 &&
               (abs_path[cwd_len] == '/' || abs_path[cwd_len] == '\0');

  free(abs_path);
  return inside;
}
