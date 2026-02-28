/*
 * Tests for utiles/hash_table.c.
 * File-based mapping: utiles/hash_table.c <-> tests/hash_table.c
 */

#include "../utiles/hash_table.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define RUN(t) do { t(); } while (0)

static void test_ht_new_nonnull(void) {
  struct HashTable *ht = ht_new(8);
  assert(ht != NULL);
  ht_free(ht);
}

static void test_ht_free_null(void) { ht_free(NULL); }

static void test_ht_add_then_get(void) {
  struct HashTable *ht = ht_new(8);
  assert(ht != NULL);
  ht_add(ht, "name", "dsc");
  const char *v = ht_get(ht, "name");
  assert(v != NULL && strcmp(v, "dsc") == 0);
  ht_free(ht);
}

static void test_ht_overwrite(void) {
  struct HashTable *ht = ht_new(8);
  assert(ht != NULL);
  ht_add(ht, "k", "v1");
  ht_add(ht, "k", "v2");
  const char *v = ht_get(ht, "k");
  assert(v != NULL && strcmp(v, "v2") == 0);
  ht_free(ht);
}

static void test_ht_remove_missing_and_existing(void) {
  struct HashTable *ht = ht_new(8);
  assert(ht != NULL);
  assert(ht_remove(ht, "missing") == HT_ERROR);
  ht_add(ht, "k", "v");
  assert(ht_remove(ht, "k") == HT_OK);
  assert(ht_get(ht, "k") == NULL);
  ht_free(ht);
}

static void test_ht_print_null_and_after_add(void) {
  ht_print(NULL);
  struct HashTable *ht = ht_new(8);
  assert(ht != NULL);
  ht_add(ht, "a", "1");
  ht_print(ht);
  ht_free(ht);
}

static void test_ht_caller_frees_key_after_add(void) {
  struct HashTable *ht = ht_new(8);
  assert(ht != NULL);
  char *k = strdup("k");
  ht_add(ht, k, "v");
  free(k);
  const char *v = ht_get(ht, "k");
  assert(v != NULL && strcmp(v, "v") == 0);
  ht_free(ht);
}

int main(void) {
  RUN(test_ht_new_nonnull);
  RUN(test_ht_free_null);
  RUN(test_ht_add_then_get);
  RUN(test_ht_overwrite);
  RUN(test_ht_remove_missing_and_existing);
  RUN(test_ht_print_null_and_after_add);
  RUN(test_ht_caller_frees_key_after_add);
  return 0;
}
