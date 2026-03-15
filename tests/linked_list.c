/*
 * Tests for utiles/linked_list.c.
 * File-based mapping: utiles/linked_list.c <-> tests/linked_list.c
 */

#include "../utiles/linked_list.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RUN(t)                                                                 \
  do {                                                                         \
    t();                                                                       \
  } while (0)

static void noop(void *v) { (void)v; }

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
  assert(ll_append(ll, NULL, "v", NULL) == LL_ERROR);
  assert(ll_append(ll, "k", NULL, NULL) == LL_ERROR);
  assert(ll_append(NULL, "k", "v", NULL) == LL_ERROR);
  assert(ll_len(ll) == 0);
  ll_free(ll);
}

static void test_ll_push_front_rejects_null(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  assert(ll_push_front(ll, NULL, "v", NULL) == LL_ERROR);
  assert(ll_push_front(ll, "k", NULL, NULL) == LL_ERROR);
  assert(ll_push_front(NULL, "k", "v", NULL) == LL_ERROR);
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
  assert(ll_pop(NULL, 1) == NULL);
  assert(ll_pop(ll, 1) == NULL);
  assert(ll_append(ll, "k", "v", NULL) == LL_OK);
  assert(ll_pop(ll, 2) == NULL);
  assert(ll_pop(ll, -1) == NULL);
  ll_free(ll);
}

static void test_ll_swap_pos_null_and_same(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  ll_append(ll, "a", "1", NULL);
  struct Node *n = (struct Node *)ll_find(ll, "a");
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
  ll_append(ll, buf, "val", NULL);
  buf[0] = 'X';
  struct Node *node = (struct Node *)ll_find(ll, "key");
  assert(node != NULL);
  assert(strcmp(ll_node_value(node), "val") == 0);
  ll_free(ll);
}

static void test_ll_caller_frees_key_after_insert(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  char *k = strdup("k");
  ll_append(ll, k, "v", NULL);
  free(k);
  struct Node *node = (struct Node *)ll_find(ll, "k");
  assert(node != NULL);
  assert(strcmp(ll_node_value(node), "v") == 0);
  ll_free(ll);
}

static void test_ll_modifying_returned_key_pointer_changes_stored_key(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  ll_append(ll, "abc", "val", NULL);
  struct Node *node = (struct Node *)ll_find(ll, "abc");
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
  assert(ll_append(ll, "k", "v", NULL) == LL_OK);
  assert(ll_len(ll) == 1);
  struct Node *node = (struct Node *)ll_find(ll, "k");
  assert(node != NULL);
  assert(strcmp(ll_node_key(node), "k") == 0);
  assert(strcmp(ll_node_value(node), "v") == 0);
  ll_free(ll);
}

static void test_ll_push_front_then_print(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  assert(ll_push_front(ll, "k", "v", NULL) == LL_OK);
  ll_print(ll);
  ll_free(ll);
}

static void test_ll_remove_existing(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  ll_append(ll, "a", "1", noop);
  ll_append(ll, "b", "2", noop);
  assert(ll_remove(ll, "b") == LL_OK);
  assert(ll_len(ll) == 1);
  assert(ll_find(ll, "b") == NULL);
  assert(ll_find(ll, "a") != NULL);
  ll_free(ll);
}

static void test_ll_pop_front_and_back(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  ll_append(ll, "first", "1", NULL);
  ll_append(ll, "mid", "2", NULL);
  ll_append(ll, "last", "3", NULL);
  assert(ll_pop(ll, LL_DIR_FRONT) != NULL);
  assert(ll_len(ll) == 2);
  assert(ll_find(ll, "first") == NULL);
  assert(ll_pop(ll, LL_DIR_BACK) != NULL);
  assert(ll_len(ll) == 1);
  assert(ll_find(ll, "last") == NULL);
  assert(ll_find(ll, "mid") != NULL);
  ll_free(ll);
}

static void test_ll_swap_pos(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  ll_append(ll, "A", "vA", NULL);
  ll_append(ll, "B", "vB", NULL);
  struct Node *na = (struct Node *)ll_find(ll, "A");
  struct Node *nb = (struct Node *)ll_find(ll, "B");
  ll_swap_pos(na, nb);
  assert(ll_find(ll, "A") != NULL &&
         strcmp(ll_node_value((struct Node *)ll_find(ll, "A")), "vA") == 0);
  assert(ll_find(ll, "B") != NULL &&
         strcmp(ll_node_value((struct Node *)ll_find(ll, "B")), "vB") == 0);
  ll_free(ll);
}

static void test_ll_append_iteration_order(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  ll_append(ll, "first",  "1", NULL);
  ll_append(ll, "second", "2", NULL);
  ll_append(ll, "third",  "3", NULL);
  assert(ll_len(ll) == 3);
  const char *expected[] = {"first", "second", "third"};
  ll_reset_iter(ll);
  int idx = 0;
  const struct Node *n;
  while ((n = ll_iter(ll)) != NULL) {
    assert(idx < 3);
    assert(strcmp(ll_node_key(n), expected[idx]) == 0);
    idx++;
  }
  assert(idx == 3);
  ll_free(ll);
}

static void test_ll_push_front_iteration_order(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  ll_push_front(ll, "a", "1", NULL);
  ll_push_front(ll, "b", "2", NULL);
  ll_push_front(ll, "c", "3", NULL);
  assert(ll_len(ll) == 3);
  /* c was pushed last so it's at the head */
  ll_reset_iter(ll);
  const struct Node *first = ll_iter(ll);
  assert(first != NULL && strcmp(ll_node_key(first), "c") == 0);
  ll_free(ll);
}

static void test_ll_remove_middle_element(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  ll_append(ll, "a", "1", NULL);
  ll_append(ll, "b", "2", NULL);
  ll_append(ll, "c", "3", NULL);
  assert(ll_remove(ll, "b") == LL_OK);
  assert(ll_len(ll) == 2);
  assert(ll_find(ll, "b") == NULL);
  assert(ll_find(ll, "a") != NULL);
  assert(ll_find(ll, "c") != NULL);
  ll_free(ll);
}

static void test_ll_iter_reset_revisits_all(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  ll_append(ll, "x", "1", NULL);
  ll_append(ll, "y", "2", NULL);
  /* first pass */
  ll_reset_iter(ll);
  int count = 0;
  while (ll_iter(ll) != NULL) count++;
  assert(count == 2);
  /* reset then second pass must yield same count */
  ll_reset_iter(ll);
  count = 0;
  while (ll_iter(ll) != NULL) count++;
  assert(count == 2);
  ll_free(ll);
}

static void test_ll_len_tracks_after_remove(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  ll_append(ll, "a", "1", NULL);
  ll_append(ll, "b", "2", NULL);
  assert(ll_len(ll) == 2);
  ll_remove(ll, "a");
  assert(ll_len(ll) == 1);
  ll_remove(ll, "b");
  assert(ll_len(ll) == 0);
  ll_free(ll);
}

static void test_ll_remove_head_and_tail(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  ll_append(ll, "head", "1", NULL);
  ll_append(ll, "mid",  "2", NULL);
  ll_append(ll, "tail", "3", NULL);
  assert(ll_remove(ll, "head") == LL_OK);
  assert(ll_remove(ll, "tail") == LL_OK);
  assert(ll_len(ll) == 1);
  assert(ll_find(ll, "mid") != NULL);
  ll_free(ll);
}

static void test_ll_find_after_multiple_inserts(void) {
  struct LinkedList *ll = ll_new();
  assert(ll != NULL);
  for (int i = 0; i < 10; i++) {
    char key[4];
    snprintf(key, sizeof(key), "k%d", i);
    ll_append(ll, key, "v", NULL);
  }
  assert(ll_len(ll) == 10);
  assert(ll_find(ll, "k0") != NULL);
  assert(ll_find(ll, "k9") != NULL);
  assert(ll_find(ll, "k5") != NULL);
  assert(ll_find(ll, "k10") == NULL);
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
  RUN(test_ll_append_iteration_order);
  RUN(test_ll_push_front_iteration_order);
  RUN(test_ll_remove_middle_element);
  RUN(test_ll_iter_reset_revisits_all);
  RUN(test_ll_len_tracks_after_remove);
  RUN(test_ll_remove_head_and_tail);
  RUN(test_ll_find_after_multiple_inserts);
  return 0;
}
