/*
 * Test suite for utiles/index.c: i_new, i_free, i_add, i_get.
 * File-based mapping: utiles/index.c <-> tests/index.c
 */

#include "../utiles/index.h"
#include "../utiles/hash.h"
#include "../utiles/hash_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int tests_run;
static int tests_passed;

static void test_start(const char *name) {
  printf("  [index] %s ... ", name);
  fflush(stdout);
  tests_run++;
}

static void test_ok(void) {
  printf("PASS\n");
  tests_passed++;
}
static void test_fail(const char *reason) { printf("FAIL (%s)\n", reason); }

static struct INode *make_root(void) {
  unsigned char *hash = calloc(HASH_LEN, 1);
  if (!hash)
    return NULL;
  struct INode *root =
      i_new(0, INDEX_MODE_DIR, INDEX_STATUS_NONE, 0, strdup("."), hash);
  if (!root) {
    free(hash);
    return NULL;
  }
  root->daughters = ht_new(HT_MAX_SIZE);
  return root;
}

static void test_i_new_null_path(void) {
  test_start("i_new with NULL path returns NULL");
  unsigned char *hash = calloc(HASH_LEN, 1);
  struct INode *n = i_new(0, INDEX_MODE_FILE, INDEX_STATUS_NONE, 0, NULL, hash);
  if (n != NULL) {
    i_free(n);
    test_fail("expected NULL");
    return;
  }
  free(hash);
  test_ok();
}

static void test_i_new_null_hash(void) {
  test_start("i_new with NULL hash returns NULL");
  char *path = strdup("x");
  struct INode *n = i_new(0, INDEX_MODE_FILE, INDEX_STATUS_NONE, 0, path, NULL);
  if (n != NULL) {
    i_free(n);
    test_fail("expected NULL");
    return;
  }
  free(path);
  test_ok();
}

static void test_i_new_valid(void) {
  test_start("i_new with valid args returns correct fields");
  unsigned char *hash = calloc(HASH_LEN, 1);
  hash[0] = 0xab;
  struct INode *n = i_new(42, INDEX_MODE_FILE, INDEX_STATUS_MODIFIED, 0,
                          strdup("./foo.c"), hash);
  if (!n) {
    test_fail("i_new returned NULL");
    return;
  }
  if (n->change_time != 42) {
    i_free(n);
    test_fail("change_time mismatch");
    return;
  }
  if (n->mode != INDEX_MODE_FILE) {
    i_free(n);
    test_fail("mode mismatch");
    return;
  }
  if (n->status != INDEX_STATUS_MODIFIED) {
    i_free(n);
    test_fail("status mismatch");
    return;
  }
  if (strcmp(n->path, "./foo.c") != 0) {
    i_free(n);
    test_fail("path mismatch");
    return;
  }
  if (n->hash[0] != 0xab) {
    i_free(n);
    test_fail("hash mismatch");
    return;
  }
  i_free(n);
  test_ok();
}

static void test_i_free_null(void) {
  test_start("i_free(NULL) does not crash");
  i_free(NULL);
  test_ok();
}

static void test_i_get_null(void) {
  test_start("i_get with NULL root returns NULL");
  if (i_get(NULL, "foo.c") != NULL) {
    test_fail("expected NULL");
    return;
  }
  test_ok();
}

static void test_i_get_missing(void) {
  test_start("i_get for missing path returns NULL");
  struct INode *root = make_root();
  if (!root) {
    test_fail("make_root failed");
    return;
  }
  if (i_get(root, "nonexistent.c") != NULL) {
    i_free(root);
    test_fail("expected NULL");
    return;
  }
  i_free(root);
  test_ok();
}

static void test_i_add_and_get(void) {
  test_start("i_add then i_get returns the node");
  struct INode *root = make_root();
  if (!root) {
    test_fail("make_root failed");
    return;
  }

  unsigned char *hash = calloc(HASH_LEN, 1);
  hash[0] = 0xcd;
  struct INode *file = i_new(99, INDEX_MODE_FILE, INDEX_STATUS_MODIFIED, 0,
                             strdup("./foo.c"), hash);
  if (!file) {
    i_free(root);
    test_fail("i_new failed");
    return;
  }

  if (i_add(root, file) != INDEX_OK) {
    i_free(file);
    i_free(root);
    test_fail("i_add failed");
    return;
  }
  file->hash = NULL; /* ownership transferred to tree */
  i_free(file);

  /* _i_add stores "foo.c" under root's daughters; i_get looks up each token */
  const struct INode *found = i_get(root, "foo.c");
  if (!found) {
    i_free(root);
    test_fail("i_get returned NULL");
    return;
  }
  if (found->mode != INDEX_MODE_FILE) {
    i_free(root);
    test_fail("mode mismatch after i_add");
    return;
  }
  i_free(root);
  test_ok();
}

static void test_i_add_nested_and_get(void) {
  test_start("i_add nested path then i_get retrieves it");
  struct INode *root = make_root();
  if (!root) {
    test_fail("make_root failed");
    return;
  }

  unsigned char *hash = calloc(HASH_LEN, 1);
  struct INode *file = i_new(0, INDEX_MODE_FILE, INDEX_STATUS_ADDED, 0,
                             strdup("./sub/foo.c"), hash);
  if (!file) {
    i_free(root);
    test_fail("i_new failed");
    return;
  }

  if (i_add(root, file) != INDEX_OK) {
    i_free(file);
    i_free(root);
    test_fail("i_add failed");
    return;
  }
  file->hash = NULL; /* ownership transferred to tree */
  i_free(file);

  const struct INode *found = i_get(root, "sub/foo.c");
  if (!found) {
    i_free(root);
    test_fail("i_get returned NULL for nested path");
    return;
  }
  i_free(root);
  test_ok();
}

/* Helper: create a file INode with a heap-allocated hash byte h0. */
static struct INode *make_file(const char *path, unsigned char h0,
                               time_t ct, short unsigned int status) {
  unsigned char *hash = calloc(HASH_LEN, 1);
  if (!hash) return NULL;
  hash[0] = h0;
  struct INode *n = i_new(ct, INDEX_MODE_FILE, status, 0, strdup(path), hash);
  if (!n) { free(hash); }
  return n;
}

static void test_i_add_null_root(void) {
  test_start("i_add(NULL, valid) returns INDEX_ERROR");
  struct INode *file = make_file("./a.c", 0x01, 0, INDEX_STATUS_ADDED);
  if (!file) { test_fail("make_file failed"); return; }
  if (i_add(NULL, file) != INDEX_ERROR) {
    i_free(file);
    test_fail("expected INDEX_ERROR");
    return;
  }
  i_free(file);
  test_ok();
}

static void test_i_add_null_inode(void) {
  test_start("i_add(valid, NULL) returns INDEX_ERROR");
  struct INode *root = make_root();
  if (!root) { test_fail("make_root failed"); return; }
  if (i_add(root, NULL) != INDEX_ERROR) {
    i_free(root);
    test_fail("expected INDEX_ERROR");
    return;
  }
  i_free(root);
  test_ok();
}

static void test_i_add_multiple_files_same_dir(void) {
  test_start("i_add multiple files in root dir - all retrievable via i_get");
  struct INode *root = make_root();
  if (!root) { test_fail("make_root failed"); return; }

  const char *paths[] = {"./foo.c", "./bar.c", "./baz.c"};
  for (int i = 0; i < 3; i++) {
    struct INode *file = make_file(paths[i], (unsigned char)(i + 1), 0,
                                   INDEX_STATUS_ADDED);
    if (!file) { i_free(root); test_fail("make_file failed"); return; }
    int ret = i_add(root, file);
    file->hash = NULL; /* ownership transferred */
    i_free(file);
    if (ret != INDEX_OK) {
      i_free(root);
      test_fail("i_add failed");
      return;
    }
  }

  if (!i_get(root, "foo.c") || !i_get(root, "bar.c") ||
      !i_get(root, "baz.c")) {
    i_free(root);
    test_fail("one or more files not found after i_add");
    return;
  }
  i_free(root);
  test_ok();
}

static void test_i_add_deeply_nested(void) {
  test_start("i_add ./a/b/c.c then i_get(root, \"a/b/c.c\") finds it");
  struct INode *root = make_root();
  if (!root) { test_fail("make_root failed"); return; }

  struct INode *file = make_file("./a/b/c.c", 0x42, 0, INDEX_STATUS_ADDED);
  if (!file) { i_free(root); test_fail("make_file failed"); return; }
  int ret = i_add(root, file);
  file->hash = NULL;
  i_free(file);
  if (ret != INDEX_OK) { i_free(root); test_fail("i_add failed"); return; }

  const struct INode *found = i_get(root, "a/b/c.c");
  if (!found) {
    i_free(root);
    test_fail("i_get returned NULL for deeply nested path");
    return;
  }
  if (found->mode != INDEX_MODE_FILE) {
    i_free(root);
    test_fail("mode mismatch for deeply nested node");
    return;
  }
  i_free(root);
  test_ok();
}

static void test_i_add_update_existing(void) {
  test_start("re-adding same path updates hash and status");
  struct INode *root = make_root();
  if (!root) { test_fail("make_root failed"); return; }

  struct INode *f1 = make_file("./up.c", 0x11, 10, INDEX_STATUS_ADDED);
  if (!f1) { i_free(root); test_fail("make_file failed"); return; }
  int ret = i_add(root, f1);
  f1->hash = NULL;
  i_free(f1);
  if (ret != INDEX_OK) { i_free(root); test_fail("first i_add failed"); return; }

  struct INode *f2 = make_file("./up.c", 0x22, 20, INDEX_STATUS_MODIFIED);
  if (!f2) { i_free(root); test_fail("make_file failed"); return; }
  ret = i_add(root, f2);
  f2->hash = NULL;
  i_free(f2);
  if (ret != INDEX_OK) { i_free(root); test_fail("second i_add failed"); return; }

  const struct INode *found = i_get(root, "up.c");
  if (!found) { i_free(root); test_fail("i_get returned NULL"); return; }
  if (found->hash[0] != 0x22) {
    i_free(root);
    test_fail("hash not updated on re-add");
    return;
  }
  if (found->status != INDEX_STATUS_MODIFIED) {
    i_free(root);
    test_fail("status not updated on re-add");
    return;
  }
  i_free(root);
  test_ok();
}

static void test_i_add_multiple_subdirs(void) {
  test_start("files in distinct subdirs are each retrievable");
  struct INode *root = make_root();
  if (!root) { test_fail("make_root failed"); return; }

  const char *paths[] = {"./sub1/a.c", "./sub2/b.c", "./sub1/c.c"};
  for (int i = 0; i < 3; i++) {
    struct INode *file = make_file(paths[i], (unsigned char)(i + 1), 0,
                                   INDEX_STATUS_ADDED);
    if (!file) { i_free(root); test_fail("make_file failed"); return; }
    int ret = i_add(root, file);
    file->hash = NULL;
    i_free(file);
    if (ret != INDEX_OK) { i_free(root); test_fail("i_add failed"); return; }
  }

  if (!i_get(root, "sub1/a.c")) {
    i_free(root); test_fail("sub1/a.c not found"); return;
  }
  if (!i_get(root, "sub2/b.c")) {
    i_free(root); test_fail("sub2/b.c not found"); return;
  }
  if (!i_get(root, "sub1/c.c")) {
    i_free(root); test_fail("sub1/c.c not found"); return;
  }
  i_free(root);
  test_ok();
}

static void test_i_add_hash_affects_root_hash(void) {
  test_start("root hash changes after i_add (recomp propagates)");
  struct INode *root = make_root();
  if (!root) { test_fail("make_root failed"); return; }

  unsigned char hash_before[HASH_LEN];
  memcpy(hash_before, root->hash, HASH_LEN);

  struct INode *file = make_file("./x.c", 0xab, 0, INDEX_STATUS_ADDED);
  if (!file) { i_free(root); test_fail("make_file failed"); return; }
  int ret = i_add(root, file);
  file->hash = NULL;
  i_free(file);
  if (ret != INDEX_OK) { i_free(root); test_fail("i_add failed"); return; }

  if (memcmp(root->hash, hash_before, HASH_LEN) == 0) {
    i_free(root);
    test_fail("root hash unchanged after adding a child");
    return;
  }
  i_free(root);
  test_ok();
}

int main(void) {
  printf("\n=== index.c tests ===\n\n");
  test_i_new_null_path();
  test_i_new_null_hash();
  test_i_new_valid();
  test_i_free_null();
  test_i_get_null();
  test_i_get_missing();
  test_i_add_and_get();
  test_i_add_nested_and_get();
  test_i_add_null_root();
  test_i_add_null_inode();
  test_i_add_multiple_files_same_dir();
  test_i_add_deeply_nested();
  test_i_add_update_existing();
  test_i_add_multiple_subdirs();
  test_i_add_hash_affects_root_hash();
  printf("\n--- Summary ---\n");
  printf("  Passed: %d / %d\n", tests_passed, tests_run);
  return tests_passed == tests_run ? 0 : 1;
}
