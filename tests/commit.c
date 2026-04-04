/*
 * Tests for sub_commands/commit.c: _hash_commit, commit.
 * File-based mapping: sub_commands/commit.c <-> tests/commit.c
 */

#include "../sub_commands/commit.h"
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

/* _hash_commit is not exported in commit.h but is a non-static symbol. */
int _hash_commit(const char *tfat, const char *tfategna,
                 const unsigned char *tree_hash, unsigned char *out);

static int tests_run;
static int tests_passed;

static void test_start(const char *name) {
  printf("  [commit] %s ... ", name);
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
  char tmpdir[] = "/tmp/berero_commit_XXXXXX";
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

/* ---- _hash_commit tests ---- */

static void test_hash_commit_null_tree_hash(void) {
  test_start("_hash_commit NULL tree_hash returns HASH_ERROR");
  unsigned char out[HASH_LEN];
  if (_hash_commit("author", "committer", NULL, out) != HASH_ERROR) {
    test_fail("expected HASH_ERROR");
    return;
  }
  test_ok();
}

static void test_hash_commit_null_tfat(void) {
  test_start("_hash_commit NULL tfat returns HASH_ERROR");
  unsigned char tree_hash[HASH_LEN] = {0};
  unsigned char out[HASH_LEN];
  if (_hash_commit(NULL, "committer", tree_hash, out) != HASH_ERROR) {
    test_fail("expected HASH_ERROR");
    return;
  }
  test_ok();
}

static void test_hash_commit_null_tfategna(void) {
  test_start("_hash_commit NULL tfategna returns HASH_ERROR");
  unsigned char tree_hash[HASH_LEN] = {0};
  unsigned char out[HASH_LEN];
  if (_hash_commit("author", NULL, tree_hash, out) != HASH_ERROR) {
    test_fail("expected HASH_ERROR");
    return;
  }
  test_ok();
}

static void test_hash_commit_null_out(void) {
  test_start("_hash_commit NULL out returns HASH_ERROR");
  unsigned char tree_hash[HASH_LEN] = {0};
  if (_hash_commit("author", "committer", tree_hash, NULL) != HASH_ERROR) {
    test_fail("expected HASH_ERROR");
    return;
  }
  test_ok();
}

static void test_hash_commit_valid(void) {
  test_start("_hash_commit with valid args returns HASH_OK");
  unsigned char tree_hash[HASH_LEN];
  memset(tree_hash, 0xAB, HASH_LEN);
  unsigned char out[HASH_LEN];
  if (_hash_commit("Author Name", "author@example.com", tree_hash, out) !=
      HASH_OK) {
    test_fail("expected HASH_OK");
    return;
  }
  test_ok();
}

static void test_hash_commit_deterministic(void) {
  test_start("_hash_commit is deterministic for identical inputs");
  unsigned char tree_hash[HASH_LEN];
  memset(tree_hash, 0x12, HASH_LEN);
  unsigned char out1[HASH_LEN], out2[HASH_LEN];
  if (_hash_commit("author", "committer", tree_hash, out1) != HASH_OK ||
      _hash_commit("author", "committer", tree_hash, out2) != HASH_OK) {
    test_fail("_hash_commit returned error");
    return;
  }
  if (memcmp(out1, out2, HASH_LEN) != 0) {
    test_fail("results differ across two calls with identical inputs");
    return;
  }
  test_ok();
}

static void test_hash_commit_distinct_authors(void) {
  test_start("_hash_commit differs when author changes");
  unsigned char tree_hash[HASH_LEN];
  memset(tree_hash, 0x34, HASH_LEN);
  unsigned char out1[HASH_LEN], out2[HASH_LEN];
  _hash_commit("author-A", "committer", tree_hash, out1);
  _hash_commit("author-B", "committer", tree_hash, out2);
  if (memcmp(out1, out2, HASH_LEN) == 0) {
    test_fail("different authors produced the same hash");
    return;
  }
  test_ok();
}

static void test_hash_commit_distinct_tree_hashes(void) {
  test_start("_hash_commit differs when tree hash changes");
  unsigned char tree_a[HASH_LEN], tree_b[HASH_LEN];
  memset(tree_a, 0xAA, HASH_LEN);
  memset(tree_b, 0xBB, HASH_LEN);
  unsigned char out1[HASH_LEN], out2[HASH_LEN];
  _hash_commit("author", "committer", tree_a, out1);
  _hash_commit("author", "committer", tree_b, out2);
  if (memcmp(out1, out2, HASH_LEN) == 0) {
    test_fail("different tree hashes produced the same commit hash");
    return;
  }
  test_ok();
}

/* ---- commit() tests ---- */

static void test_commit_wrong_argc(void) {
  test_start("commit(argc=2) returns COMMIT_ERROR");
  char *argv[] = {"berero", "author-only", NULL};
  if (commit(2, argv) != COMMIT_ERROR) {
    test_fail("expected COMMIT_ERROR");
    return;
  }
  test_ok();
}

static void test_commit_no_index(void) {
  test_start("commit with no index file returns COMMIT_ERROR");
  if (setup_tmpdir() != 0) {
    test_fail("setup_tmpdir failed");
    return;
  }
  /* .berero/index does not exist. */
  char *argv[] = {"berero", "Author Name", "author@example.com", NULL};
  int ret = commit(3, argv);
  teardown_tmpdir();
  if (ret != COMMIT_ERROR) {
    test_fail("expected COMMIT_ERROR when index is absent");
    return;
  }
  test_ok();
}

static void test_commit_empty_index(void) {
  test_start("commit with empty index (no staged files) returns COMMIT_ERROR");
  if (setup_tmpdir() != 0) {
    test_fail("setup_tmpdir failed");
    return;
  }

  FILE *f = fopen(INDEX_FILE_PATH, "wb");
  if (!f) {
    teardown_tmpdir();
    test_fail("could not create index file");
    return;
  }
  fclose(f);

  char *argv[] = {"berero", "Author Name", "author@example.com", NULL};
  int ret = commit(3, argv);
  teardown_tmpdir();
  if (ret != COMMIT_ERROR) {
    test_fail("expected COMMIT_ERROR for empty index");
    return;
  }
  test_ok();
}

static void test_commit_full_integration(void) {
  test_start("commit with valid staged file returns COMMIT_OK and writes head");
  if (setup_tmpdir() != 0) {
    test_fail("setup_tmpdir failed");
    return;
  }

  /* Write a source file. */
  const char *content = "hello berero\n";
  FILE *src = fopen("src.txt", "wb");
  if (!src) {
    teardown_tmpdir();
    test_fail("could not create src.txt");
    return;
  }
  fwrite(content, 1, strlen(content), src);
  fclose(src);

  /* Hash it. */
  unsigned char hash[HASH_LEN];
  src = fopen("src.txt", "rb");
  if (!src || sh_hash(src, hash) != HASH_OK) {
    if (src)
      fclose(src);
    teardown_tmpdir();
    test_fail("sh_hash failed");
    return;
  }
  fclose(src);

  char hex[HASH_LEN * 2 + 1];
  sh_bin_to_hex(hash, HASH_LEN, hex);

  /* Store the blob. */
  if (bwrite("src.txt", hex) != BLOB_OK) {
    teardown_tmpdir();
    test_fail("bwrite failed");
    return;
  }

  /* Build the index tree. */
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

  unsigned char *fhash = malloc(HASH_LEN);
  if (!fhash) {
    i_free(root);
    teardown_tmpdir();
    test_fail("malloc failed");
    return;
  }
  memcpy(fhash, hash, HASH_LEN);
  struct INode *file = i_new(0, INDEX_MODE_FILE, INDEX_STATUS_ADDED, 0,
                              strdup("./src.txt"), fhash);
  if (!file) {
    free(fhash);
    i_free(root);
    teardown_tmpdir();
    test_fail("i_new for file failed");
    return;
  }

  if (i_add(root, file) != INDEX_OK) {
    i_free(file);
    i_free(root);
    teardown_tmpdir();
    test_fail("i_add failed");
    return;
  }
  file->hash = NULL; /* ownership transferred to tree */
  i_free(file);

  if (i_dump(root) != INDEX_OK) {
    i_free(root);
    teardown_tmpdir();
    test_fail("i_dump failed");
    return;
  }
  i_free(root);

  /* Run commit. */
  char *argv[] = {"berero", "Test Author", "test@example.com", NULL};
  int ret = commit(3, argv);
  int head_exists = (access(COMMIT_HEAD_FILE, F_OK) == 0);
  teardown_tmpdir();

  if (ret != COMMIT_OK) {
    test_fail("commit returned non-COMMIT_OK");
    return;
  }
  if (!head_exists) {
    test_fail("COMMIT_HEAD_FILE was not written");
    return;
  }
  test_ok();
}

static void test_commit_second_commit_writes_parent(void) {
  test_start("second commit writes 'parent' line referencing first commit hash");
  if (setup_tmpdir() != 0) {
    test_fail("setup_tmpdir failed");
    return;
  }

  /* Create and stage a file. */
  const char *content = "v1\n";
  FILE *src = fopen("a.txt", "wb");
  if (!src) { teardown_tmpdir(); test_fail("fopen failed"); return; }
  fwrite(content, 1, strlen(content), src);
  fclose(src);

  unsigned char hash[HASH_LEN];
  src = fopen("a.txt", "rb");
  if (!src || sh_hash(src, hash) != HASH_OK) {
    if (src) fclose(src);
    teardown_tmpdir();
    test_fail("sh_hash failed");
    return;
  }
  fclose(src);

  char hex[HASH_LEN * 2 + 1];
  sh_bin_to_hex(hash, HASH_LEN, hex);
  if (bwrite("a.txt", hex) != BLOB_OK) {
    teardown_tmpdir();
    test_fail("bwrite failed");
    return;
  }

  unsigned char *rh = calloc(HASH_LEN, 1);
  if (!rh) { teardown_tmpdir(); test_fail("calloc"); return; }
  struct INode *root =
      i_new(0, INDEX_MODE_DIR, INDEX_STATUS_NONE, 0, strdup("."), rh);
  if (!root) { free(rh); teardown_tmpdir(); test_fail("i_new"); return; }
  root->daughters = ht_new(HT_MAX_SIZE);

  unsigned char *fh = malloc(HASH_LEN);
  if (!fh) { i_free(root); teardown_tmpdir(); test_fail("malloc"); return; }
  memcpy(fh, hash, HASH_LEN);
  struct INode *file =
      i_new(0, INDEX_MODE_FILE, INDEX_STATUS_ADDED, 0, strdup("./a.txt"), fh);
  if (!file) { free(fh); i_free(root); teardown_tmpdir(); test_fail("i_new file"); return; }
  if (i_add(root, file) != INDEX_OK) {
    i_free(file); i_free(root); teardown_tmpdir(); test_fail("i_add"); return;
  }
  file->hash = NULL;
  i_free(file);
  if (i_dump(root) != INDEX_OK) {
    i_free(root); teardown_tmpdir(); test_fail("i_dump"); return;
  }
  i_free(root);

  /* First commit. */
  char *argv[] = {"berero", "Author", "author@example.com", NULL};
  if (commit(3, argv) != COMMIT_OK) {
    teardown_tmpdir();
    test_fail("first commit failed");
    return;
  }

  /* Read first commit hash from head. */
  FILE *head = fopen(COMMIT_HEAD_FILE, "rb");
  if (!head) { teardown_tmpdir(); test_fail("open head failed"); return; }
  char first_hash[HASH_LEN * 2 + 2];
  if (fscanf(head, "%65s", first_hash) != 1) {
    fclose(head);
    teardown_tmpdir();
    test_fail("read head failed");
    return;
  }
  fclose(head);

  /* Second commit with the same index (re-dump it). */
  rh = calloc(HASH_LEN, 1);
  if (!rh) { teardown_tmpdir(); test_fail("calloc 2"); return; }
  root = i_new(0, INDEX_MODE_DIR, INDEX_STATUS_NONE, 0, strdup("."), rh);
  if (!root) { free(rh); teardown_tmpdir(); test_fail("i_new 2"); return; }
  root->daughters = ht_new(HT_MAX_SIZE);
  fh = malloc(HASH_LEN);
  if (!fh) { i_free(root); teardown_tmpdir(); test_fail("malloc 2"); return; }
  memcpy(fh, hash, HASH_LEN);
  file = i_new(0, INDEX_MODE_FILE, INDEX_STATUS_ADDED, 0, strdup("./a.txt"), fh);
  if (!file) { free(fh); i_free(root); teardown_tmpdir(); test_fail("i_new file 2"); return; }
  if (i_add(root, file) != INDEX_OK) {
    i_free(file); i_free(root); teardown_tmpdir(); test_fail("i_add 2"); return;
  }
  file->hash = NULL;
  i_free(file);
  if (i_dump(root) != INDEX_OK) {
    i_free(root); teardown_tmpdir(); test_fail("i_dump 2"); return;
  }
  i_free(root);

  if (commit(3, argv) != COMMIT_OK) {
    teardown_tmpdir();
    test_fail("second commit failed");
    return;
  }

  /* Read the second commit object and verify it contains a 'parent' line. */
  head = fopen(COMMIT_HEAD_FILE, "rb");
  if (!head) { teardown_tmpdir(); test_fail("open head 2 failed"); return; }
  char second_hash[HASH_LEN * 2 + 2];
  if (fscanf(head, "%65s", second_hash) != 1) {
    fclose(head);
    teardown_tmpdir();
    test_fail("read head 2 failed");
    return;
  }
  fclose(head);

  /* Open the second commit object file and look for the parent line. */
  char obj_dir[BLOB_OBJECT_DIR_LEN];
  char obj_file[BLOB_OBJECT_FILE_LEN];
  object_file_dir_location(second_hash, obj_dir);
  object_file_location(second_hash, obj_file);

  FILE *obj = fopen(obj_file, "rb");
  if (!obj) { teardown_tmpdir(); test_fail("open commit object failed"); return; }

  char line[256];
  int found_parent = 0;
  while (fgets(line, sizeof(line), obj)) {
    if (strncmp(line, "parent ", 7) == 0) {
      /* Verify the parent hash matches the first commit. */
      char *parent_hash = line + 7;
      /* Strip trailing newline. */
      size_t l = strlen(parent_hash);
      if (l > 0 && parent_hash[l - 1] == '\n')
        parent_hash[l - 1] = '\0';
      if (strncmp(parent_hash, first_hash, HASH_LEN * 2) == 0)
        found_parent = 1;
      break;
    }
  }
  fclose(obj);
  teardown_tmpdir();

  if (!found_parent) {
    test_fail("second commit object does not contain correct 'parent' line");
    return;
  }
  test_ok();
}

int main(void) {
  printf("\n=== commit.c tests ===\n\n");
  printf("--- _hash_commit ---\n");
  test_hash_commit_null_tree_hash();
  test_hash_commit_null_tfat();
  test_hash_commit_null_tfategna();
  test_hash_commit_null_out();
  test_hash_commit_valid();
  test_hash_commit_deterministic();
  test_hash_commit_distinct_authors();
  test_hash_commit_distinct_tree_hashes();
  printf("--- commit ---\n");
  test_commit_wrong_argc();
  test_commit_no_index();
  test_commit_empty_index();
  test_commit_full_integration();
  test_commit_second_commit_writes_parent();
  printf("\n--- Summary ---\n");
  printf("  Passed: %d / %d\n", tests_passed, tests_run);
  return tests_passed == tests_run ? 0 : 1;
}
