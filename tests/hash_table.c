/*
 * Tests for utiles/hash_table.c.
 * File-based mapping: utiles/hash_table.c <-> tests/hash_table.c
 */

#include "../utiles/hash_table.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define RUN(t)                                                                 \
  do {                                                                         \
    t();                                                                       \
  } while (0)

static void noop(void *v) { (void)v; }

static void test_ht_new_nonnull(void) {
  struct HashTable *ht = ht_new(8);
  assert(ht != NULL);
  ht_free(ht);
}

static void test_ht_free_null(void) { ht_free(NULL); }

static void test_ht_add_then_get(void) {
  struct HashTable *ht = ht_new(8);
  assert(ht != NULL);
  ht_add(ht, "name", "dsc", NULL);
  const char *v = ht_get(ht, "name");
  assert(v != NULL && strcmp(v, "dsc") == 0);
  ht_free(ht);
}

static void test_ht_add_same_key_get_returns_latest(void) {
  struct HashTable *ht = ht_new(8);
  assert(ht != NULL);
  ht_add(ht, "k", "v1", NULL);
  ht_add(ht, "k", "v2", NULL);
  const char *v = ht_get(ht, "k");
  /* ll_push_front puts v2 at head; ll_find searches from head */
  assert(v != NULL && strcmp(v, "v2") == 0);
  ht_free(ht);
}

static void test_ht_remove_missing_and_existing(void) {
  struct HashTable *ht = ht_new(8);
  assert(ht != NULL);
  assert(ht_remove(ht, "missing") == HT_ERROR);
  ht_add(ht, "k", "v", noop);
  assert(ht_remove(ht, "k") == HT_OK);
  assert(ht_get(ht, "k") == NULL);
  ht_free(ht);
}

static void test_ht_print_null_and_after_add(void) {
  ht_print(NULL);
  struct HashTable *ht = ht_new(8);
  assert(ht != NULL);
  ht_add(ht, "a", "1", NULL);
  ht_print(ht);
  ht_free(ht);
}

static void test_ht_caller_frees_key_after_add(void) {
  struct HashTable *ht = ht_new(8);
  assert(ht != NULL);
  char *k = strdup("k");
  ht_add(ht, k, "v", NULL);
  free(k);
  const char *v = ht_get(ht, "k");
  assert(v != NULL && strcmp(v, "v") == 0);
  ht_free(ht);
}

static void test_ht_new_size_one(void) {
  struct HashTable *ht = ht_new(1);
  assert(ht != NULL);
  ht_add(ht, "key", "val", NULL);
  const char *v = ht_get(ht, "key");
  assert(v != NULL && strcmp(v, "val") == 0);
  ht_free(ht);
}

static void test_ht_get_on_empty_table(void) {
  struct HashTable *ht = ht_new(8);
  assert(ht != NULL);
  assert(ht_get(ht, "anything") == NULL);
  ht_free(ht);
}

static void test_ht_get_null_key(void) {
  struct HashTable *ht = ht_new(8);
  assert(ht != NULL);
  assert(ht_get(ht, NULL) == NULL);
  ht_free(ht);
}

static void test_ht_multiple_distinct_keys(void) {
  struct HashTable *ht = ht_new(8);
  assert(ht != NULL);
  ht_add(ht, "alpha", "1", NULL);
  ht_add(ht, "beta",  "2", NULL);
  ht_add(ht, "gamma", "3", NULL);
  assert(strcmp(ht_get(ht, "alpha"), "1") == 0);
  assert(strcmp(ht_get(ht, "beta"),  "2") == 0);
  assert(strcmp(ht_get(ht, "gamma"), "3") == 0);
  /* adding new keys does not disturb existing ones */
  ht_add(ht, "delta", "4", NULL);
  assert(strcmp(ht_get(ht, "alpha"), "1") == 0);
  ht_free(ht);
}

static void test_ht_iter_visits_all_entries(void) {
  struct HashTable *ht = ht_new(8);
  assert(ht != NULL);
  ht_add(ht, "x", "1", NULL);
  ht_add(ht, "y", "2", NULL);
  ht_add(ht, "z", "3", NULL);
  int count = 0;
  ht_reset_iter(ht);
  while (ht_iter(ht) != NULL)
    count++;
  assert(count == 3);
  /* second pass after reset */
  count = 0;
  ht_reset_iter(ht);
  while (ht_iter(ht) != NULL)
    count++;
  assert(count == 3);
  ht_free(ht);
}

static void test_ht_remove_then_readd(void) {
  struct HashTable *ht = ht_new(8);
  assert(ht != NULL);
  ht_add(ht, "k", "v1", noop);
  assert(ht_remove(ht, "k") == HT_OK);
  assert(ht_get(ht, "k") == NULL);
  ht_add(ht, "k", "v2", NULL);
  const char *v = ht_get(ht, "k");
  assert(v != NULL && strcmp(v, "v2") == 0);
  ht_free(ht);
}

static void test_ht_stringify_nonnull(void) {
  struct HashTable *ht = ht_new(4);
  assert(ht != NULL);
  ht_add(ht, "key", "val", NULL);
  char *s = ht_stringify(ht);
  assert(s != NULL);
  free(s);
  ht_free(ht);
}

static void test_ht_stringify_empty(void) {
  struct HashTable *ht = ht_new(4);
  assert(ht != NULL);
  char *s = ht_stringify(ht);
  assert(s != NULL); /* empty table still returns allocated "" */
  free(s);
  ht_free(ht);
}

int main(void) {
  RUN(test_ht_new_nonnull);
  RUN(test_ht_free_null);
  RUN(test_ht_add_then_get);
  RUN(test_ht_add_same_key_get_returns_latest);
  RUN(test_ht_remove_missing_and_existing);
  RUN(test_ht_print_null_and_after_add);
  RUN(test_ht_caller_frees_key_after_add);
  RUN(test_ht_new_size_one);
  RUN(test_ht_get_on_empty_table);
  RUN(test_ht_get_null_key);
  RUN(test_ht_multiple_distinct_keys);
  RUN(test_ht_iter_visits_all_entries);
  RUN(test_ht_remove_then_readd);
  RUN(test_ht_stringify_nonnull);
  RUN(test_ht_stringify_empty);
  return 0;
}
