/*
 * Tests for utiles/walker.c.
 * File-based mapping: utiles/walker.c <-> tests/walker.c
 */

#include "../utiles/walker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int tests_run;
static int tests_passed;

static void test_start(const char *name) {
  printf("  [walker] %s ... ", name);
  fflush(stdout);
  tests_run++;
}

static void test_ok(void) {
  printf("PASS\n");
  tests_passed++;
}

static void test_fail(const char *reason) { printf("FAIL (%s)\n", reason); }

static void test_walk_null_path(void) {
  test_start("walk(NULL) returns NULL");
  struct LinkedList *list = walk(NULL);
  if (list != NULL) {
    ll_free(list);
    test_fail("expected NULL");
    return;
  }
  test_ok();
}

static void test_walk_nonexistent_path(void) {
  test_start("walk on nonexistent path returns NULL");
  struct LinkedList *list = walk("/nonexistent/path/xyz");
  if (list != NULL) {
    ll_free(list);
    test_fail("expected NULL");
    return;
  }
  test_ok();
}

static void test_walk_valid_path(void) {
  test_start("walk on valid directory returns non-NULL list");
  struct LinkedList *list = walk("/tmp");
  if (list == NULL) {
    test_fail("expected non-NULL list");
    return;
  }
  ll_free(list);
  test_ok();
}

static void test_walk_entries_have_keys(void) {
  test_start("walk list entries have non-NULL keys");
  struct LinkedList *list = walk("./");
  if (list == NULL) {
    test_fail("walk(/tmp) returned NULL");
    return;
  }
  ll_reset_iter(list);
  struct Node *node;
  while ((node = ll_iter(list)) != NULL) {
    if (ll_node_key(node) == NULL) {
      ll_free(list);
      test_fail("entry has NULL key");
      return;
    }
  }
  ll_free(list);
  test_ok();
}

static void test_walk_file_path_returns_only_that_file(void) {
  test_start("walk on file path returns that file as only entry");
  char path[] = "/tmp/berero_walker_test_XXXXXX";
  int fd = mkstemp(path);
  if (fd < 0) {
    test_fail("mkstemp failed");
    return;
  }
  close(fd);

  struct LinkedList *list = walk(path);
  unlink(path);

  if (list == NULL) {
    test_fail("walk(file) returned NULL");
    return;
  }
  ll_reset_iter(list);
  struct Node *node;
  const char *only_key = NULL;
  int count = 0;
  while ((node = ll_iter(list)) != NULL) {
    if (ll_node_key(node) != NULL) {
      only_key = ll_node_key(node);
      count++;
    }
  }
  if (count != 1) {
    ll_free(list);
    test_fail("expected exactly one entry");
    return;
  }
  if (only_key == NULL || strcmp(only_key, path) != 0) {
    ll_free(list);
    test_fail("single entry should be the file path");
    return;
  }
  ll_free(list);
  test_ok();
}

int main(void) {
  printf("\n=== walker.c tests ===\n\n");
  test_walk_null_path();
  test_walk_nonexistent_path();
  test_walk_valid_path();
  test_walk_entries_have_keys();
  test_walk_file_path_returns_only_that_file();
  printf("\n  Passed: %d / %d\n", tests_passed, tests_run);
  return tests_passed == tests_run ? 0 : 1;
}
