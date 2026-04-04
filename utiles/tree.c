#include "tree.h"
#include "blob.h"
#include "hash.h"
#include "index.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>
#include <zlib.h>

int dump_tree(const struct INode *i, const char *path_prefix) {
  if (!i)
    return TREE_ERROR;

  char *path;
  if (path_prefix[0] == '\0') {
    path = strdup(i->path);
  } else {
    path = malloc(strlen(path_prefix) + 1 + strlen(i->path) + 1);
    if (!path)
      return TREE_ERROR;
    snprintf(path, strlen(path_prefix) + 1 + strlen(i->path) + 1, "%s/%s",
             path_prefix, i->path);
  }
  if (!path)
    return TREE_ERROR;

  char *hash = malloc(HASH_LEN * 2 + 1);
  if (!hash) {
    free(path);
    return TREE_ERROR;
  }
  sh_bin_to_hex(i->hash, HASH_LEN, hash);

  if (i->mode == INDEX_MODE_FILE) {
    // make sure the file exists
    char file_out[BLOB_OBJECT_FILE_LEN];
    if (object_file_location(hash, file_out) != BLOB_OK) {
      free(path);
      free(hash);
      return TREE_ERROR;
    }

    if (access(file_out, F_OK) != 0) {
      free(path);
      free(hash);
      return TREE_ERROR;
    }

    free(path);
    free(hash);
    return TREE_OK;
  }

  if (i->n_daughters == 0) {
    free(path);
    free(hash);
    return TREE_OK;
  }

  ht_reset_iter(i->daughters);
  const struct INode *curr;
  while ((curr = ht_iter(i->daughters)) != NULL) {
    if (dump_tree(curr, path) != TREE_OK) {
      free(path);
      free(hash);
      return TREE_ERROR;
    }
  }

  // save tree with the hash path
  char dir_out[BLOB_OBJECT_DIR_LEN];
  if (object_file_dir_location(hash, dir_out) != BLOB_OK) {
    free(path);
    free(hash);
    return TREE_ERROR;
  }
  if (mkdir(dir_out, BLOB_DIR_PRIVATE_MODE) == -1 && errno != EEXIST) {
    perror(dir_out);
    free(path);
    free(hash);
    return TREE_ERROR;
  }

  char file_out[BLOB_OBJECT_FILE_LEN];
  if (object_file_location(hash, file_out) != TREE_OK) {
    free(path);
    free(hash);
    return TREE_ERROR;
  }
  FILE *f = fopen(file_out, "wb");
  if (!f) {
    perror(file_out);
    free(path);
    free(hash);
    return TREE_ERROR;
  }

  ht_reset_iter(i->daughters);
  const struct INode *child;
  while ((child = ht_iter(i->daughters)) != NULL) {
    char child_hash[HASH_LEN * 2 + 1];
    sh_bin_to_hex(child->hash, HASH_LEN, child_hash);

    // format: "<mode> <hash> <path>\n"
    if (fprintf(f, "%u %s %s\n", child->mode, child_hash, child->path) < 0) {
      fclose(f);
      free(path);
      free(hash);
      return TREE_ERROR;
    }
  }

  fclose(f);
  free(path);
  free(hash);
  return TREE_OK;
}
