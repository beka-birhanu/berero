#ifndef INDEX_H
#define INDEX_H

#include "hash_table.h"
#include <time.h>

#define INDEX_OK 0
#define INDEX_ERROR -1

#define INDEX_STATUS_ADDED 1
#define INDEX_STATUS_DELETED 2
#define INDEX_STATUS_MODIFIED 3

#define INDEX_FILE_PATH "./.berero/index"

struct Index {
  time_t change_time;
  char *file_name;
  char *hash;
  unsigned int status;
};

/* Load the index file. Returns NULL on failure. */
struct HashTable *im_load();

/* Dump the index file. Returns INDEX_OK on success, INDEX_ERROR on failure. */
int im_dump(struct HashTable *ht);

/* Return the index for key, or NULL if not found or on error. */
const struct Index *i_get(struct HashTable *ht, const char *key);

/* Add the index to the hash table. Returns INDEX_OK on success, INDEX_ERROR on
 * failure. a copy of index is stored; caller retains ownership. */
int i_add(struct HashTable *ht, const struct Index *index);

/* Allocate a new index. Returns NULL on failure. */
struct Index *i_new(const char *file_name, const char *hash, time_t change_time,
                    unsigned int status);

/* Free the index and all its fields. */
void i_free(struct Index *idx);

#endif
