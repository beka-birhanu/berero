#include "index.h"
#include "hash.h"
#include "hash_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int _i_add(struct INode *curr, const struct INode *inode, char *path_tok);
int _recomp_hash(struct INode *curr);
void _i_free(void *i);
int _i_dump(const struct INode *i, FILE *f, const char *path_prifix);

struct INode *i_new(time_t change_time, short unsigned int mode,
                    short unsigned int status, unsigned int n_daughters,
                    char *path, unsigned char *hash) {
  if (path == NULL || hash == NULL)
    return NULL;
  struct INode *i = malloc(sizeof(struct INode));
  i->change_time = change_time;
  i->mode = mode;
  i->status = status;
  i->n_daughters = n_daughters;
  i->path = path;
  i->hash = hash;
  return i;
}

void _i_free(void *i) { return i == NULL ?: i_free(i); }
void i_free(struct INode *i) {
  if (i == NULL)
    return;
  free(i->path);
  free(i->hash);
  ht_free(i->daughters);
  free(i);
}

int i_add(struct INode *itree, const struct INode *inode) {
  if (itree == NULL || inode == NULL || itree->hash == NULL ||
      inode->hash == NULL || itree->path == NULL || inode->path == NULL)
    return INDEX_ERROR;
  char *path = strdup(inode->path);
  if (path == NULL)
    return INDEX_ERROR;
  char *path_tok = strtok(path, "/");
  if (path_tok == NULL) {
    free(path);
    return INDEX_ERROR;
  }

  if (_i_add(itree, inode, path_tok) != INDEX_OK) {
    free(path);
    return INDEX_ERROR;
  }

  free(path);
  return INDEX_OK;
}

int _i_add(struct INode *curr, const struct INode *inode, char *path_tok) {
  if (curr == NULL || inode == NULL)
    return INDEX_OK;

  path_tok = strtok(NULL, "/");
  if (path_tok == NULL) {
    curr->status = inode->status;
    curr->mode = inode->mode;
    curr->n_daughters = inode->n_daughters;
    free(curr->hash);
    curr->hash = inode->hash;
    return INDEX_OK;
  }

  if (curr->daughters == NULL) {
    curr->daughters = ht_new(HT_MAX_SIZE);
  }

  if (ht_get(curr->daughters, path_tok) == NULL) {
    unsigned char *hash = malloc(HASH_LEN);
    if (!hash) {
      return INDEX_ERROR;
    }
    struct INode *new = i_new(inode->change_time, INDEX_MODE_DIR,
                              INDEX_STATUS_NONE, 0, strdup(path_tok), hash);
    ht_add(curr->daughters, path_tok, new, _i_free);
    curr->n_daughters++;
    curr->change_time = curr->change_time < inode->change_time
                            ? inode->change_time
                            : curr->change_time;
  }

  if (_i_add(ht_get(curr->daughters, path_tok), inode, path_tok) != INDEX_OK) {
    return INDEX_ERROR;
  }

  return _recomp_hash(curr);
}

int _recomp_hash(struct INode *curr) {
  if (curr == NULL)
    return INDEX_ERROR;

  if (curr->n_daughters == 0)
    return INDEX_OK;

  const unsigned char **hashes =
      malloc(curr->n_daughters * sizeof(unsigned char *));
  if (!hashes)
    return INDEX_ERROR;

  ht_reset_iter(curr->daughters);
  for (unsigned int i = 0; i < curr->n_daughters; i++) {
    const struct INode *inode = ht_iter(curr->daughters);
    if (!inode)
      return INDEX_ERROR;
    hashes[i] = inode->hash;
  }

  unsigned char *out = malloc(HASH_LEN);
  if (!out) {
    free(hashes);
    return INDEX_ERROR;
  }

  if (sh_combine_hash(hashes, curr->n_daughters, out) != HASH_OK) {
    free(hashes);
    return INDEX_ERROR;
  }

  free(curr->hash);
  curr->hash = out;
  free(hashes);
  return INDEX_OK;
}

void i_print(const struct INode *curr) {
  if (curr == NULL)
    return;

  char hex[HASH_LEN * 2 + 1];
  sh_bin_to_hex(curr->hash, HASH_LEN, hex);
  printf("path: %s\n", curr->path);
  printf("hash: %s\n", hex);
  printf("\n");
  printf("mode: %d\n", curr->mode);
  printf("status: %d\n", curr->status);
  printf("n_daughters: %d\n", curr->n_daughters);

  ht_reset_iter(curr->daughters);
  for (unsigned int i = 0; i < curr->n_daughters; i++) {
    const struct INode *inode = ht_iter(curr->daughters);
    i_print(inode);
  }
}

const struct INode *i_get(const struct INode *i, const char *_path) {
  if (!i || !_path)
    return NULL;

  char *path = strdup(_path);
  if (!path)
    return NULL;
  char *path_tok = strtok(path, "/");
  if (!path_tok) {
    free(path);
    return NULL;
  }

  while (path_tok) {
    const struct INode *inode = ht_get(i->daughters, path_tok);
    if (!inode) {
      free(path);
      return NULL;
    }
    i = inode;
    path_tok = strtok(NULL, "/");
  }

  free(path);
  return i;
}

struct INode *i_load() {
  FILE *f = fopen(INDEX_FILE_PATH, "rb");
  if (!f)
    return NULL;

  unsigned char *root_hash = calloc(HASH_LEN, 1);
  if (!root_hash) {
    fclose(f);
    return NULL;
  }
  struct INode *root =
      i_new(0, INDEX_MODE_DIR, INDEX_STATUS_NONE, 0, strdup("."), root_hash);
  if (!root) {
    free(root_hash);
    fclose(f);
    return NULL;
  }
  root->daughters = ht_new(HT_MAX_SIZE);

  char path[4096];
  char hex[HASH_LEN * 2 + 1];
  long change_time;
  unsigned int status;

  while (fscanf(f, "%4095s %64s %ld %u", path, hex, &change_time, &status) ==
         4) {
    unsigned char *hash = malloc(HASH_LEN);
    if (!hash) {
      i_free(root);
      fclose(f);
      return NULL;
    }
    for (int j = 0; j < HASH_LEN; j++) {
      unsigned int byte;
      sscanf(hex + j * 2, "%02x", &byte);
      hash[j] = (unsigned char)byte;
    }

    struct INode *node =
        i_new((time_t)change_time, INDEX_MODE_FILE, (short unsigned int)status,
              0, strdup(path), hash);
    if (!node) {
      free(hash);
      i_free(root);
      fclose(f);
      return NULL;
    }
    node->daughters = NULL;

    if (i_add(root, node) != INDEX_OK) {
      node->hash = NULL;
      i_free(node);
      i_free(root);
      fclose(f);
      return NULL;
    }
    // hash ownership is transferred to root
    node->hash = NULL;
    i_free(node);
  }

  fclose(f);
  return root;
}

int i_dump(const struct INode *i) {
  if (!i)
    return INDEX_ERROR;

  FILE *f = fopen(INDEX_FILE_PATH, "wb");
  if (!f) {
    perror(INDEX_FILE_PATH);
    return INDEX_ERROR;
  }

  if (_i_dump(i, f, "") != INDEX_OK) {
    fclose(f);
    return INDEX_ERROR;
  }

  fclose(f);
  return INDEX_OK;
}

int _i_dump(const struct INode *i, FILE *f, const char *path_prifix) {
  if (!i || !f)
    return INDEX_ERROR;

  char *path;
  if (path_prifix[0] == '\0') {
    path = strdup(i->path);
  } else {
    path = malloc(strlen(path_prifix) + 1 + strlen(i->path) + 1);
    if (!path)
      return INDEX_ERROR;
    sprintf(path, "%s/%s", path_prifix, i->path);
  }
  if (!path)
    return INDEX_ERROR;

  if (i->mode == INDEX_MODE_FILE) {
    char hex[HASH_LEN * 2 + 1];
    sh_bin_to_hex(i->hash, HASH_LEN, hex);
    if (fprintf(f, "%s %s %ld %u\n", path, hex, (long)i->change_time,
                (unsigned)i->status) < 0) {
      free(path);
      return INDEX_ERROR;
    }
  }

  ht_reset_iter(i->daughters);
  const struct INode *curr;
  while ((curr = ht_iter(i->daughters)) != NULL) {
    if (_i_dump(curr, f, path) != INDEX_OK) {
      free(path);
      return INDEX_ERROR;
    }
  }

  free(path);
  return INDEX_OK;
}
