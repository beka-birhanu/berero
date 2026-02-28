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

static void hash_to_hex(const unsigned char *hash, unsigned int len,
                        char *out) {
  if (!hash || !out)
    return;
  for (unsigned int i = 0; i < len; i++)
    sprintf(out + (i * 2), "%02x", hash[i]);
  out[len * 2] = '\0';
}

int add(int argc, char *argv[]) {
  if (argc < 2) {
    help(argc, argv);
    return ADD_ERROR;
  }

  struct HashTable *idx_map = im_load();
  if (!idx_map) {
    perror(INDEX_FILE_PATH);
    return ADD_ERROR;
  }

  while (--argc > 0) {
    const char *path = *++argv;
    if (!is_path_in_cwd(path)) {
      fprintf(stderr,
              "Warning: path %s is not in the current working directory\n",
              path);
      ht_free(idx_map);
      return ADD_ERROR;
    }

    struct LinkedList *list = walk(path);
    if (!list) {
      ht_free(idx_map);
      return ADD_ERROR;
    }

    ll_reset_iter(list);
    struct Node *node;
    while ((node = ll_iter(list)) != NULL) {
      const char *key = ll_node_key(node);
      if (!key) {
        ll_free(list);
        ht_free(idx_map);
        return ADD_ERROR;
      }

      struct stat st;
      if (stat(key, &st) == -1) {
        perror(key);
        ll_free(list);
        ht_free(idx_map);
        return ADD_ERROR;
      }

      const struct Index *idx = i_get(idx_map, key);
      if (idx && idx->change_time == st.st_mtime)
        continue;

      FILE *file = fopen(key, "rb");
      if (!file) {
        perror(key);
        ll_free(list);
        ht_free(idx_map);
        return ADD_ERROR;
      }

      unsigned char hash_bin[EVP_MAX_MD_SIZE];
      if (sh_hash(file, hash_bin) != HASH_OK) {
        fclose(file);
        ll_free(list);
        ht_free(idx_map);
        return ADD_ERROR;
      }
      fclose(file);

      /* SHA-256 = 32 bytes */
      char hash_hex[65];
      hash_to_hex(hash_bin, 32, hash_hex);
      if ((strcmp(idx ? idx->hash : "", hash_hex) == 0))
        continue;

      if (bwrite(key, hash_hex) != BLOB_OK) {
        ll_free(list);
        ht_free(idx_map);
        return ADD_ERROR;
      }

      i_add(idx_map, i_new(key, hash_hex, st.st_mtime,
                           idx ? INDEX_STATUS_MODIFIED : INDEX_STATUS_ADDED));
    }
    ll_free(list);
  }

  im_dump(idx_map);
  ht_free(idx_map);

  return ADD_OK;
}
