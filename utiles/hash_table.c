#include "hash_table.h"
#include "linked_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct HashTable {
  size_t size;
  size_t _current_bucket;
  struct LinkedList *table[];
};

/* FNV-1a hash of key, reduced to [0, table_size). */
static size_t fnv1a_hash(const char *key, size_t table_size) {
  unsigned long hash = 14695981039346656037UL;
  if (!key)
    return 0;
  while (*key) {
    hash ^= (unsigned char)(*key++);
    hash *= 1099511628211UL;
  }
  return hash % table_size;
}

struct HashTable *ht_new(size_t size) {
  if (size < HT_MIN_SIZE || size > HT_MAX_SIZE)
    return NULL;

  struct HashTable *ht =
      malloc(sizeof(*ht) + sizeof(struct LinkedList *) * size);
  if (!ht)
    return NULL;

  ht->size = size;
  for (size_t i = 0; i < size; i++)
    ht->table[i] = ll_new();

  ht->_current_bucket = 0;

  return ht;
}

void *ht_get(const struct HashTable *ht, const char *key) {
  if (!ht || !key)
    return NULL;
  size_t hash = fnv1a_hash(key, ht->size);
  struct LinkedList *ll = ht->table[hash];

  if (!ll)
    return NULL;

  const struct Node *node = ll_find(ll, key);
  if (!node)
    return NULL;

  return ll_node_value(node);
}

int ht_remove(const struct HashTable *ht, const char *key) {
  if (!ht || !key)
    return HT_ERROR;
  size_t hash = fnv1a_hash(key, ht->size);
  struct LinkedList *ll = ht->table[hash];

  if (!ll)
    return HT_ERROR;

  return ll_remove(ll, key) == LL_OK ? HT_OK : HT_ERROR;
}

int ht_add(const struct HashTable *ht, const char *key, void *value,
           value_destructor destructor) {
  if (!ht || !key)
    return HT_ERROR;
  size_t hash = fnv1a_hash(key, ht->size);
  struct LinkedList *ll = ht->table[hash];

  if (!ll)
    return HT_ERROR;

  return ll_push_front(ll, key, value, destructor) == LL_OK ? HT_OK : HT_ERROR;
}

const void *ht_iter(struct HashTable *ht) {
  const struct Node *node = NULL;

  if (!ht)
    return NULL;

  while (!node && ht->_current_bucket < ht->size) {
    struct LinkedList *ll = ht->table[ht->_current_bucket];
    if (!ll) {
      return NULL;
    }

    node = ll_iter(ll);
    if (!node)
      ht->_current_bucket++;
  }

  return ll_node_value(node);
}

/* Reset the iterator to the beginning of the hash table.*/
void ht_reset_iter(struct HashTable *ht) {
  if (!ht)
    return;

  for (size_t i = 0; i < ht->size; i++)
    ll_reset_iter(ht->table[i]);

  ht->_current_bucket = 0;
}

void ht_free(struct HashTable *ht) {
  if (!ht)
    return;

  for (size_t i = 0; i < ht->size; i++) {
    struct LinkedList *ll = ht->table[i];
    if (!ll)
      continue;

    ll_free(ll);
  }

  free(ht);
}

char *ht_stringify(const struct HashTable *ht) {
  if (!ht)
    return NULL;

  size_t total = 0;
  char header[HT_MAX_HEADER_LEN];
  for (size_t i = 0; i < ht->size; i++) {
    struct LinkedList *ll = ht->table[i];
    if (ll_len(ll) == 0)
      continue;

    int hlen = snprintf(header, sizeof(header), "[bucket %zu]\n", i);
    char *lls = ll_stringify(ll);
    total += (size_t)hlen + (lls ? strlen(lls) : 0);
    free(lls);
  }

  char *out = malloc(total + 1);
  if (!out)
    return NULL;
  if (total == 0) {
    *out = '\0';
    return out;
  }

  char *p = out;
  for (size_t i = 0; i < ht->size; i++) {
    struct LinkedList *ll = ht->table[i];
    if (ll_len(ll) == 0)
      continue;

    size_t hlen = (size_t)snprintf(header, sizeof(header), "[bucket %zu]\n", i);
    memcpy(p, header, hlen);
    p += hlen;
    char *lls = ll_stringify(ll);
    if (lls) {
      size_t llen = strlen(lls);
      memcpy(p, lls, llen + 1);
      p += llen;
      *(p++) = '\n';
      free(lls);
    }
  }
  *p = '\0';
  return out;
}

void ht_print(const struct HashTable *ht) {
  char *s = ht_stringify(ht);
  if (s) {
    fputs(s, stdout);
    fputc('\n', stdout);
    free(s);
  }
}
