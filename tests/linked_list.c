/*
 * Tests for utiles/linked_list.c.
 * File-based mapping: utiles/linked_list.c <-> tests/linked_list.c
 */

#include "../utiles/linked_list.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define RUN(t) do { t(); } while (0)

static void test_ll_new_nonnull_and_len_zero(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  assert(ll_len(ll) == 0);
  ll_free(ll);
}

static void test_ll_free_null(void) { ll_free(NULL); }

static void test_ll_append_rejects_null(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  assert(ll_append(ll, NULL, "v") == LL_ERROR);
  assert(ll_append(ll, "k", NULL) == LL_ERROR);
  assert(ll_append(NULL, "k", "v") == LL_ERROR);
  assert(ll_len(ll) == 0);
  ll_free(ll);
}

static void test_ll_push_front_rejects_null(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  assert(ll_push_front(ll, NULL, "v") == LL_ERROR);
  assert(ll_push_front(ll, "k", NULL) == LL_ERROR);
  assert(ll_push_front(NULL, "k", "v") == LL_ERROR);
  assert(ll_len(ll) == 0);
  ll_free(ll);
}

static void test_ll_len_null(void) { assert(ll_len(NULL) == 0); }

static void test_ll_find_null(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  assert(ll_find(NULL, "k") == NULL);
  assert(ll_find(ll, NULL) == NULL);
  ll_free(ll);
}

static void test_ll_remove_null_and_missing(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  assert(ll_remove(NULL, "k") == LL_ERROR);
  assert(ll_remove(ll, NULL) == LL_ERROR);
  assert(ll_remove(ll, "missing") == LL_ERROR);
  ll_free(ll);
}

static void test_ll_pop_invalid(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  assert(ll_pop(NULL, 1) == LL_ERROR);
  assert(ll_pop(ll, 1) == LL_ERROR);
  assert(ll_append(ll, "k", "v") == LL_OK);
  assert(ll_pop(ll, 2) == LL_ERROR);
  assert(ll_pop(ll, -1) == LL_ERROR);
  ll_free(ll);
}

static void test_ll_swap_pos_null_and_same(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  ll_append(ll, "a", "1");
  struct Node *n = ll_find(ll, "a");
  ll_swap_pos(NULL, n);
  ll_swap_pos(n, NULL);
  ll_swap_pos(n, n);
  ll_free(ll);
}

static void test_ll_node_key_value_null(void) {
  assert(ll_node_key(NULL) == NULL);
  assert(ll_node_value(NULL) == NULL);
}

static void test_ll_print_null(void) { ll_print(NULL); }

static void test_ll_caller_modifies_buffer_after_insert(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  char buf[] = "key";
  ll_append(ll, buf, "val");
  buf[0] = 'X';
  struct Node *node = ll_find(ll, "key");
  assert(node != NULL);
  assert(strcmp(ll_node_value(node), "val") == 0);
  ll_free(ll);
}

static void test_ll_caller_frees_key_after_insert(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  char *k = strdup("k");
  ll_append(ll, k, "v");
  free(k);
  struct Node *node = ll_find(ll, "k");
  assert(node != NULL);
  assert(strcmp(ll_node_value(node), "v") == 0);
  ll_free(ll);
}

static void test_ll_modifying_returned_key_pointer_changes_stored_key(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  ll_append(ll, "abc", "val");
  struct Node *node = ll_find(ll, "abc");
  assert(node != NULL);
  const char *p = ll_node_key(node);
  ((char *)p)[0] = 'X';
  assert(ll_find(ll, "Xbc") != NULL);
  assert(ll_find(ll, "abc") == NULL);
  ll_free(ll);
}

static void test_ll_append_then_len_find_accessors(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  assert(ll_append(ll, "k", "v") == LL_OK);
  assert(ll_len(ll) == 1);
  struct Node *node = ll_find(ll, "k");
  assert(node != NULL);
  assert(strcmp(ll_node_key(node), "k") == 0);
  assert(strcmp(ll_node_value(node), "v") == 0);
  ll_free(ll);
}

static void test_ll_push_front_then_print(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  assert(ll_push_front(ll, "k", "v") == LL_OK);
  ll_print(ll);
  ll_free(ll);
}

static void test_ll_remove_existing(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  ll_append(ll, "a", "1");
  ll_append(ll, "b", "2");
  assert(ll_remove(ll, "b") == LL_OK);
  assert(ll_len(ll) == 1);
  assert(ll_find(ll, "b") == NULL);
  assert(ll_find(ll, "a") != NULL);
  ll_free(ll);
}

static void test_ll_pop_front_and_back(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  ll_append(ll, "first", "1");
  ll_append(ll, "mid", "2");
  ll_append(ll, "last", "3");
  assert(ll_pop(ll, 1) == LL_OK);
  assert(ll_len(ll) == 2);
  assert(ll_find(ll, "first") == NULL);
  assert(ll_pop(ll, 0) == LL_OK);
  assert(ll_len(ll) == 1);
  assert(ll_find(ll, "last") == NULL);
  assert(ll_find(ll, "mid") != NULL);
  ll_free(ll);
}

static void test_ll_swap_pos(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  ll_append(ll, "A", "vA");
  ll_append(ll, "B", "vB");
  struct Node *na = ll_find(ll, "A");
  struct Node *nb = ll_find(ll, "B");
  ll_swap_pos(na, nb);
  assert(ll_find(ll, "A") != NULL &&
         strcmp(ll_node_value(ll_find(ll, "A")), "vA") == 0);
  assert(ll_find(ll, "B") != NULL &&
         strcmp(ll_node_value(ll_find(ll, "B")), "vB") == 0);
  ll_free(ll);
}

int main(void) {
  RUN(test_ll_new_nonnull_and_len_zero);
  RUN(test_ll_free_null);
  RUN(test_ll_append_rejects_null);
  RUN(test_ll_push_front_rejects_null);
  RUN(test_ll_len_null);
  RUN(test_ll_find_null);
  RUN(test_ll_remove_null_and_missing);
  RUN(test_ll_pop_invalid);
  RUN(test_ll_swap_pos_null_and_same);
  RUN(test_ll_node_key_value_null);
  RUN(test_ll_print_null);
  RUN(test_ll_caller_modifies_buffer_after_insert);
  RUN(test_ll_caller_frees_key_after_insert);
  RUN(test_ll_modifying_returned_key_pointer_changes_stored_key);
  RUN(test_ll_append_then_len_find_accessors);
  RUN(test_ll_push_front_then_print);
  RUN(test_ll_remove_existing);
  RUN(test_ll_pop_front_and_back);
  RUN(test_ll_swap_pos);
  return 0;
}
