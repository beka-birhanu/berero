/*
 * Tests for utiles/walker.c.
 * File-based mapping: utiles/walker.c <-> tests/walker.c
 */

#include "../utiles/walker.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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
  const struct Node *node;
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
  const struct Node *node;
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

static void test_walk_empty_directory(void) {
  test_start("walk on empty directory returns list with zero entries");
  char tmpdir[] = "/tmp/berero_walk_empty_XXXXXX";
  if (!mkdtemp(tmpdir)) {
    test_fail("mkdtemp failed");
    return;
  }
  struct LinkedList *list = walk(tmpdir);
  rmdir(tmpdir);
  if (!list) {
    test_fail("walk returned NULL for empty dir");
    return;
  }
  ll_reset_iter(list);
  int count = 0;
  while (ll_iter(list) != NULL) count++;
  ll_free(list);
  if (count != 0) {
    test_fail("expected 0 entries for empty directory");
    return;
  }
  test_ok();
}

static void test_walk_dir_returns_all_files(void) {
  test_start("walk on directory with 3 files returns exactly 3 entries");
  char tmpdir[] = "/tmp/berero_walk_files_XXXXXX";
  if (!mkdtemp(tmpdir)) {
    test_fail("mkdtemp failed");
    return;
  }
  char paths[3][PATH_MAX];
  for (int i = 0; i < 3; i++) {
    snprintf(paths[i], sizeof(paths[i]), "%s/file%d.txt", tmpdir, i);
    FILE *f = fopen(paths[i], "w");
    if (f) { fputs("x", f); fclose(f); }
  }
  struct LinkedList *list = walk(tmpdir);
  for (int i = 0; i < 3; i++) unlink(paths[i]);
  rmdir(tmpdir);

  if (!list) { test_fail("walk returned NULL"); return; }
  ll_reset_iter(list);
  int count = 0;
  while (ll_iter(list) != NULL) count++;
  ll_free(list);
  if (count != 3) {
    test_fail("expected exactly 3 entries");
    return;
  }
  test_ok();
}

static void test_walk_nested_dir_returns_all_files(void) {
  test_start("walk on nested directory returns files from all levels");
  char tmpdir[] = "/tmp/berero_walk_nested_XXXXXX";
  if (!mkdtemp(tmpdir)) {
    test_fail("mkdtemp failed");
    return;
  }
  char subdir[PATH_MAX];
  snprintf(subdir, sizeof(subdir), "%s/sub", tmpdir);
  mkdir(subdir, 0700);

  char f1[PATH_MAX], f2[PATH_MAX];
  snprintf(f1, sizeof(f1), "%s/root.txt", tmpdir);
  snprintf(f2, sizeof(f2), "%s/nested.txt", subdir);
  FILE *fp;
  fp = fopen(f1, "w"); if (fp) { fputs("r", fp); fclose(fp); }
  fp = fopen(f2, "w"); if (fp) { fputs("n", fp); fclose(fp); }

  struct LinkedList *list = walk(tmpdir);
  unlink(f1); unlink(f2); rmdir(subdir); rmdir(tmpdir);

  if (!list) { test_fail("walk returned NULL for nested dir"); return; }
  ll_reset_iter(list);
  int count = 0;
  while (ll_iter(list) != NULL) count++;
  ll_free(list);
  if (count != 2) {
    test_fail("expected 2 entries (1 root-level + 1 nested)");
    return;
  }
  test_ok();
}

static void test_walk_absolute_path_key_unchanged(void) {
  test_start("walk on absolute file path keeps key as-is");
  char path[] = "/tmp/berero_abs_XXXXXX";
  int fd = mkstemp(path);
  if (fd < 0) { test_fail("mkstemp failed"); return; }
  close(fd);

  struct LinkedList *list = walk(path);
  unlink(path);

  if (!list) { test_fail("walk returned NULL"); return; }
  ll_reset_iter(list);
  const struct Node *node = ll_iter(list);
  if (!node) { ll_free(list); test_fail("empty list"); return; }
  const char *key = ll_node_key(node);
  if (!key || strcmp(key, path) != 0) {
    ll_free(list);
    test_fail("absolute path key was altered");
    return;
  }
  ll_free(list);
  test_ok();
}

static void test_is_path_in_cwd_null(void) {
  test_start("is_path_in_cwd(NULL) returns 0");
  if (is_path_in_cwd(NULL) != 0) {
    test_fail("expected 0 for NULL");
    return;
  }
  test_ok();
}

static void test_is_path_in_cwd_current_dir(void) {
  test_start("is_path_in_cwd(\".\") returns 1");
  if (is_path_in_cwd(".") != 1) {
    test_fail("expected 1 for current directory");
    return;
  }
  test_ok();
}

static void test_is_path_in_cwd_outside(void) {
  test_start("is_path_in_cwd(\"/\") returns 0");
  /* root is never inside a non-root cwd */
  char cwd[PATH_MAX];
  if (!getcwd(cwd, sizeof(cwd))) {
    test_fail("getcwd failed");
    return;
  }
  if (strcmp(cwd, "/") == 0) {
    /* running from root - skip */
    tests_run--;
    return;
  }
  if (is_path_in_cwd("/") != 0) {
    test_fail("expected 0 for filesystem root");
    return;
  }
  test_ok();
}

int main(void) {
  printf("\n=== walker.c tests ===\n\n");
  test_walk_null_path();
  test_walk_nonexistent_path();
  test_walk_valid_path();
  test_walk_entries_have_keys();
  test_walk_file_path_returns_only_that_file();
  test_walk_empty_directory();
  test_walk_dir_returns_all_files();
  test_walk_nested_dir_returns_all_files();
  test_walk_absolute_path_key_unchanged();
  test_is_path_in_cwd_null();
  test_is_path_in_cwd_current_dir();
  test_is_path_in_cwd_outside();
  printf("\n  Passed: %d / %d\n", tests_passed, tests_run);
  return tests_passed == tests_run ? 0 : 1;
}
