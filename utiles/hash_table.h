#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stddef.h>

#define HT_MAX_SIZE 10000
#define HT_MIN_SIZE 1
#define HT_MAX_HEADER_LEN 15

#define HT_OK 0
#define HT_ERROR -1

typedef void (*value_destructor)(void *value);

struct HashTable;

/* Allocate a new hash table with the given number of buckets. Returns NULL on
 * failure. */
struct HashTable *ht_new(size_t size);

/* Return the value associated with key, or NULL if not found or on error. */
void *ht_get(const struct HashTable *ht, const char *key);

/* Remove the entry for key. Returns HT_OK on success, HT_ERROR if not found or
 * on error. */
int ht_remove(const struct HashTable *ht, const char *key);

/* Insert or overwrite key with value. Returns HT_OK on success, HT_ERROR on
 * failure. value is stored as-is; caller looses ownership. Returns HT_ERROR on
 * failure. */
int ht_add(const struct HashTable *ht, const char *key, void *value,
           value_destructor destructor);

/* Return the next entry in the hash table, or NULL if none. */
const void *ht_iter(struct HashTable *ht);

/* Reset the iterator to the beginning of the hash table.*/
void ht_reset_iter(struct HashTable *ht);

/* Free the hash table and all its entries. */
void ht_free(struct HashTable *ht);

/* Return a newly allocated string representation (per bucket "[bucket i]\n"
 * then ll_stringify of that bucket), or NULL if ht is NULL or allocation
 * fails. Caller must free() the result. Empty table returns allocated "". */
char *ht_stringify(const struct HashTable *ht);

/* Print each bucket and its entries to stdout. No-op if ht is NULL. */
void ht_print(const struct HashTable *ht);

#endif /* HASH_TABLE_H */
