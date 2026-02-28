#include "index.h"
#include "hash_table.h"
#include "linked_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct Index *i_new(const char *file_name, const char *hash, time_t change_time,
                    unsigned int status) {

  if (!file_name || !hash)
    return NULL;

  if ((status != INDEX_STATUS_ADDED && status != INDEX_STATUS_DELETED &&
       status != INDEX_STATUS_MODIFIED)) {
    return NULL;
  }

  if (strlen(file_name) > FILENAME_MAX || strlen(hash) > 64)
    return NULL;

  struct Index *idx = malloc(sizeof(*idx));
  if (!idx)
    return NULL;

  idx->change_time = change_time;

  idx->file_name = malloc(strlen(file_name) + 1);
  if (!idx->file_name) {
    free(idx);
    return NULL;
  }
  strcpy(idx->file_name, file_name);

  idx->hash = malloc(strlen(hash) + 1);
  if (!idx->hash) {
    free(idx->file_name);
    free(idx);
    return NULL;
  }
  strcpy(idx->hash, hash);
  idx->status = status;

  return idx;
}

void i_free(struct Index *idx) {
  if (!idx)
    return;
  free(idx->file_name);
  free(idx->hash);
  free(idx);
}

char *i_stringify(const struct Index *idx) {
  if (!idx)
    return NULL;

  /* estimate size safely */
  size_t needed = snprintf(NULL, 0, "%s %ld %s %du\n", idx->file_name,
                           (long)idx->change_time, idx->hash, idx->status);

  char *s = malloc(needed + 1);
  if (!s)
    return NULL;

  snprintf(s, needed + 1, "%s %ld %s %du\n", idx->file_name,
           (long)idx->change_time, idx->hash, idx->status);
  return s;
}

struct Index *i_load(const char *s) {
  if (!s)
    return NULL;

  long change_time;
  unsigned int status;
  char file_name[FILENAME_MAX + 1];
  char hash[65]; // assuming SHA-256 hex (64 + null)

  if (sscanf(s, "%1024s %ld %64s %du", file_name, &change_time, hash,
             &status) != 3) {
    return NULL;
  }

  return i_new(file_name, hash, (time_t)change_time, status);
}

struct HashTable *im_load() {
  struct HashTable *ht = ht_new(HT_MAX_SIZE);
  if (!ht)
    return NULL;

  FILE *index_file = fopen(INDEX_FILE_PATH, "r");
  if (!index_file) {
    ht_free(ht);
    return NULL;
  }

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;

  while ((linelen = getline(&line, &linecap, index_file)) != -1) {
    // Remove trailing newline
    if (linelen > 0 && line[linelen - 1] == '\n') {
      line[linelen - 1] = '\0';
    }

    struct Index *idx = i_load(line);
    if (!idx)
      continue; // skip malformed lines

    ht_add(ht, idx->file_name, idx);
  }

  free(line);
  fclose(index_file);

  return ht;
}

int im_dump(struct HashTable *ht) {
  if (!ht)
    return INDEX_ERROR;

  FILE *index_file = fopen(INDEX_FILE_PATH, "w");
  if (!index_file) {
    perror(INDEX_FILE_PATH);
    return INDEX_ERROR;
  }

  struct Node *idx_node = NULL;
  struct Index *idx = NULL;
  char *s = NULL;

  ht_reset_iter(ht);

  while ((idx_node = ht_iter(ht))) {
    idx = ll_node_value(idx_node);
    s = i_stringify(idx);
    if (!s) {
      fclose(index_file);
      return INDEX_ERROR;
    }

    if (fputs(s, index_file) == EOF) {
      perror(INDEX_FILE_PATH);
      free(s);
      fclose(index_file);
      return INDEX_ERROR;
    }

    free(s);
  }

  if (fclose(index_file) == EOF) {
    perror(INDEX_FILE_PATH);
    return INDEX_ERROR;
  }

  return INDEX_OK;
}

const struct Index *i_get(struct HashTable *ht, const char *key) {
  if (!(ht && key))
    return NULL;

  return (struct Index *)ht_get(ht, key);
}

int i_add(struct HashTable *ht, const struct Index *index) {
  if (!(ht && index))
    return INDEX_ERROR;

  struct Index *idx_copy =
      i_new(index->file_name, index->hash, index->change_time, index->status);
  if (!idx_copy)
    return INDEX_ERROR;

  return ht_add(ht, index->file_name, idx_copy) == HT_OK ? INDEX_OK
                                                         : INDEX_ERROR;
}
