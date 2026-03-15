#include "add.h"
#include "../utiles/blob.h"
#include "../utiles/hash.h"
#include "../utiles/index.h"
#include "../utiles/walker.h"
#include "help.h"
#include <openssl/evp.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

int add(int argc, char *argv[]) {
  if (argc < 2) {
    help(argc, argv);
    return ADD_ERROR;
  }

  struct INode *idx_root = i_load();
  if (!idx_root) {
    perror(INDEX_FILE_PATH);
    return ADD_ERROR;
  }

  while (--argc > 0) {
    const char *path = *++argv;
    if (!is_path_in_cwd(path)) {
      fprintf(stderr,
              "Warning: path %s is not in the current working directory\n",
              path);
      i_free(idx_root);
      return ADD_ERROR;
    }

    struct LinkedList *list = walk(path);
    if (!list) {
      i_free(idx_root);
      return ADD_ERROR;
    }

    ll_reset_iter(list);
    const struct Node *node;
    while ((node = ll_iter(list)) != NULL) {
      const char *key = ll_node_key(node);
      if (!key) {
        ll_free(list);
        i_free(idx_root);
        return ADD_ERROR;
      }

      struct stat st;
      if (stat(key, &st) == -1) {
        perror(key);
        ll_free(list);
        i_free(idx_root);
        return ADD_ERROR;
      }

      const struct INode *idx = i_get(idx_root, key);
      if (idx && idx->mode != INDEX_MODE_FILE)
        continue;
      if (idx && idx->change_time == st.st_mtime)
        continue;

      FILE *file = fopen(key, "rb");
      if (!file) {
        perror(key);
        ll_free(list);
        i_free(idx_root);
        return ADD_ERROR;
      }

      unsigned char *hash_bin = malloc(HASH_LEN);
      if (!hash_bin) {
        fclose(file);
        ll_free(list);
        i_free(idx_root);
        return ADD_ERROR;
      }
      if (sh_hash(file, hash_bin) != HASH_OK) {
        fclose(file);
        ll_free(list);
        i_free(idx_root);
        return ADD_ERROR;
      }
      fclose(file);

      if (idx && memcmp(idx->hash, hash_bin, HASH_LEN) == 0)
        continue;

      char hash_hex[HASH_LEN * 2 + 1];
      sh_bin_to_hex(hash_bin, HASH_LEN, hash_hex);
      if (bwrite(key, hash_hex) != BLOB_OK) {
        ll_free(list);
        i_free(idx_root);
        return ADD_ERROR;
      }

      i_add(idx_root, i_new(st.st_mtime, INDEX_MODE_FILE, INDEX_STATUS_MODIFIED,
                            0, strdup(key), hash_bin));
    }
    ll_free(list);
  }
  i_dump(idx_root);
  i_free(idx_root);

  return ADD_OK;
}
