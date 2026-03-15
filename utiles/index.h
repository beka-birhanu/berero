#ifndef INDEX_H
#define INDEX_H

#include "hash_table.h"
#include <time.h>

#define INDEX_OK 0
#define INDEX_ERROR -1

#define INDEX_STATUS_NONE 0
#define INDEX_STATUS_ADDED 1
#define INDEX_STATUS_DELETED 2
#define INDEX_STATUS_MODIFIED 3

#define INDEX_FILE_PATH "./.berero/index"

#define INDEX_MODE_FILE 1
#define INDEX_MODE_DIR 2

struct INode {
  time_t change_time;
  short unsigned int mode;
  short unsigned int status;
  unsigned int n_daughters;
  char *path;
  unsigned char *hash;
  struct HashTable *daughters;
};

/* Load the index file. Returns NULL on failure. */
struct INode *i_load();

/* Dump the index file. Returns INDEX_OK on success, INDEX_ERROR on failure. */
int i_dump(const struct INode *i);

/* Return the index for path, or NULL if not found or on error. */
const struct INode *i_get(const struct INode *i, const char *_path);

/* Add the index node to the index tree. Returns INDEX_OK on success,
 * INDEX_ERROR on failure. a copy of index node is stored; caller retains
 * ownership. */
int i_add(struct INode *i, const struct INode *index);

/* Allocate a new index. Returns NULL on failure. The caller is responsible for
 * freeing the returned pointer. */
struct INode *i_new(time_t change_time, short unsigned int mode,
                    short unsigned int status, unsigned int n_daughters,
                    char *path, unsigned char *hash);

/* Free the index node and all of its daughters. */
void i_free(struct INode *i);

void i_print(const struct INode *curr);

#endif
