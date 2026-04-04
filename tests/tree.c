/*
 * Tests for utiles/tree.c: dump_tree.
 * File-based mapping: utiles/tree.c <-> tests/tree.c
 */

#include "../utiles/tree.h"
#include "../utiles/blob.h"
#include "../utiles/hash.h"
#include "../utiles/index.h"
#include "../utiles/hash_table.h"
#include "../sub_commands/init.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int tests_run;
static int tests_passed;

static void test_start(const char *name) {
  printf("  [tree] %s ... ", name);
  fflush(stdout);
  tests_run++;
}

static void test_ok(void) {
  printf("PASS\n");
  tests_passed++;
}

static void test_fail(const char *reason) { printf("FAIL (%s)\n", reason); }

/* ---- helpers ---- */

static char saved_cwd[PATH_MAX];

static int setup_tmpdir(void) {
  if (!getcwd(saved_cwd, sizeof(saved_cwd)))
    return -1;
  char tmpdir[] = "/tmp/berero_tree_XXXXXX";
  if (!mkdtemp(tmpdir))
    return -1;
  if (chdir(tmpdir) != 0)
    return -1;
  if (mkdir(INIT_ENTRY_DIR, 0700) != 0)
    return -1;
  if (mkdir(INIT_OBJECT_DIR, 0700) != 0)
    return -1;
  return 0;
}

static void teardown_tmpdir(void) { chdir(saved_cwd); }

/* Create a tiny placeholder blob file for the given hex hash in objects/. */
static int make_fake_blob(const char *hex) {
  char dir[BLOB_OBJECT_DIR_LEN];
  char file[BLOB_OBJECT_FILE_LEN];
  if (object_file_dir_location(hex, dir) != BLOB_OK)
    return -1;
  if (mkdir(dir, 0700) != 0 && errno != EEXIST)
    return -1;
  if (object_file_location(hex, file) != BLOB_OK)
    return -1;
  FILE *f = fopen(file, "wb");
  if (!f)
    return -1;
  fwrite("x", 1, 1, f);
  fclose(f);
  return 0;
}

/* Build a detached FILE INode with freshly allocated hash bytes. */
static struct INode *make_file_inode(const char *path,
                                     const unsigned char *hash_bytes) {
  unsigned char *h = malloc(HASH_LEN);
  if (!h)
    return NULL;
  memcpy(h, hash_bytes, HASH_LEN);
  struct INode *n =
      i_new(0, INDEX_MODE_FILE, INDEX_STATUS_ADDED, 0, strdup(path), h);
  if (!n)
    free(h);
  return n;
}

/* ---- tests ---- */

static void test_dump_tree_null(void) {
  test_start("dump_tree(NULL, \"\") returns TREE_ERROR");
  if (dump_tree(NULL, "") != TREE_ERROR) {
    test_fail("expected TREE_ERROR");
    return;
  }
  test_ok();
}

static void test_dump_tree_file_missing_blob(void) {
  test_start("FILE mode inode whose blob is absent returns TREE_ERROR");
  if (setup_tmpdir() != 0) {
    test_fail("setup_tmpdir failed");
    return;
  }

  /* All-0xBB hash, but we do NOT create the corresponding blob. */
  unsigned char hash[HASH_LEN];
  memset(hash, 0xBB, HASH_LEN);
  struct INode *n = make_file_inode("file.c", hash);
  if (!n) {
    teardown_tmpdir();
    test_fail("make_file_inode failed");
    return;
  }

  int ret = dump_tree(n, "");
  i_free(n);
  teardown_tmpdir();

  if (ret != TREE_ERROR) {
    test_fail("expected TREE_ERROR when blob is absent");
    return;
  }
  test_ok();
}

static void test_dump_tree_file_existing_blob(void) {
  test_start("FILE mode inode with existing blob returns TREE_OK");
  if (setup_tmpdir() != 0) {
    test_fail("setup_tmpdir failed");
    return;
  }

  unsigned char hash[HASH_LEN];
  memset(hash, 0xCC, HASH_LEN);
  char hex[HASH_LEN * 2 + 1];
  sh_bin_to_hex(hash, HASH_LEN, hex);

  if (make_fake_blob(hex) != 0) {
    teardown_tmpdir();
    test_fail("make_fake_blob failed");
    return;
  }

  struct INode *n = make_file_inode("file.c", hash);
  if (!n) {
    teardown_tmpdir();
    test_fail("make_file_inode failed");
    return;
  }

  int ret = dump_tree(n, "");
  i_free(n);
  teardown_tmpdir();

  if (ret != TREE_OK) {
    test_fail("expected TREE_OK");
    return;
  }
  test_ok();
}

static void test_dump_tree_dir_no_daughters(void) {
  test_start("DIR mode inode with 0 daughters returns TREE_OK (no I/O)");
  /* No filesystem needed — the empty-dir early exit fires before any I/O. */
  unsigned char *hash = calloc(HASH_LEN, 1);
  if (!hash) {
    test_fail("calloc failed");
    return;
  }
  struct INode *dir =
      i_new(0, INDEX_MODE_DIR, INDEX_STATUS_NONE, 0, strdup("."), hash);
  if (!dir) {
    free(hash);
    test_fail("i_new failed");
    return;
  }
  dir->daughters = ht_new(HT_MAX_SIZE);

  int ret = dump_tree(dir, "");
  i_free(dir);

  if (ret != TREE_OK) {
    test_fail("expected TREE_OK for empty directory");
    return;
  }
  test_ok();
}

static void test_dump_tree_dir_with_file_child(void) {
  test_start("DIR with one FILE child (blob present) returns TREE_OK");
  if (setup_tmpdir() != 0) {
    test_fail("setup_tmpdir failed");
    return;
  }

  unsigned char file_hash[HASH_LEN];
  memset(file_hash, 0xDD, HASH_LEN);
  char hex[HASH_LEN * 2 + 1];
  sh_bin_to_hex(file_hash, HASH_LEN, hex);

  if (make_fake_blob(hex) != 0) {
    teardown_tmpdir();
    test_fail("make_fake_blob failed");
    return;
  }

  /* Build root dir. */
  unsigned char *root_hash = calloc(HASH_LEN, 1);
  if (!root_hash) {
    teardown_tmpdir();
    test_fail("calloc failed");
    return;
  }
  struct INode *root =
      i_new(0, INDEX_MODE_DIR, INDEX_STATUS_NONE, 0, strdup("."), root_hash);
  if (!root) {
    free(root_hash);
    teardown_tmpdir();
    test_fail("i_new for root failed");
    return;
  }
  root->daughters = ht_new(HT_MAX_SIZE);

  /* Build and add file child via i_add (sets up hashtable properly). */
  struct INode *file = make_file_inode("./file.c", file_hash);
  if (!file) {
    i_free(root);
    teardown_tmpdir();
    test_fail("make_file_inode failed");
    return;
  }
  if (i_add(root, file) != INDEX_OK) {
    i_free(file);
    i_free(root);
    teardown_tmpdir();
    test_fail("i_add failed");
    return;
  }
  file->hash = NULL; /* ownership transferred to the tree */
  i_free(file);

  int ret = dump_tree(root, "");
  i_free(root);
  teardown_tmpdir();

  if (ret != TREE_OK) {
    test_fail("expected TREE_OK with file child and existing blob");
    return;
  }
  test_ok();
}

static void test_dump_tree_nested_dirs(void) {
  test_start("nested dir ./sub/file.c with blob present returns TREE_OK");
  if (setup_tmpdir() != 0) {
    test_fail("setup_tmpdir failed");
    return;
  }

  unsigned char file_hash[HASH_LEN];
  memset(file_hash, 0xEE, HASH_LEN);
  char hex[HASH_LEN * 2 + 1];
  sh_bin_to_hex(file_hash, HASH_LEN, hex);

  if (make_fake_blob(hex) != 0) {
    teardown_tmpdir();
    test_fail("make_fake_blob failed");
    return;
  }

  unsigned char *root_hash = calloc(HASH_LEN, 1);
  if (!root_hash) {
    teardown_tmpdir();
    test_fail("calloc failed");
    return;
  }
  struct INode *root =
      i_new(0, INDEX_MODE_DIR, INDEX_STATUS_NONE, 0, strdup("."), root_hash);
  if (!root) {
    free(root_hash);
    teardown_tmpdir();
    test_fail("i_new for root failed");
    return;
  }
  root->daughters = ht_new(HT_MAX_SIZE);

  struct INode *file = make_file_inode("./sub/file.c", file_hash);
  if (!file) {
    i_free(root);
    teardown_tmpdir();
    test_fail("make_file_inode failed");
    return;
  }
  if (i_add(root, file) != INDEX_OK) {
    i_free(file);
    i_free(root);
    teardown_tmpdir();
    test_fail("i_add failed");
    return;
  }
  file->hash = NULL;
  i_free(file);

  int ret = dump_tree(root, "");
  i_free(root);
  teardown_tmpdir();

  if (ret != TREE_OK) {
    test_fail("expected TREE_OK for nested path");
    return;
  }
  test_ok();
}

int main(void) {
  printf("\n=== tree.c tests ===\n\n");
  test_dump_tree_null();
  test_dump_tree_file_missing_blob();
  test_dump_tree_file_existing_blob();
  test_dump_tree_dir_no_daughters();
  test_dump_tree_dir_with_file_child();
  test_dump_tree_nested_dirs();
  printf("\n--- Summary ---\n");
  printf("  Passed: %d / %d\n", tests_passed, tests_run);
  return tests_passed == tests_run ? 0 : 1;
}
