/*
 * Test suite for utiles/index.c: i_new, i_free, i_load, load, get.
 * File-based mapping: utiles/index.c <-> tests/index.c
 *
 * Note: this intentionally avoids dump() because current hash table iteration
 * behavior can cause dump() to hang; we are not fixing bugs here.
 */

#include "../utiles/index.h"
#include "../utiles/hash_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* i_load is implemented in index.c but not declared in index.h */
extern struct Index *i_load(const char *s);

static int tests_run;
static int tests_passed;

static void test_start(const char *suite, const char *name) {
  printf("  [%s] %s ... ", suite, name);
  fflush(stdout);
  tests_run++;
}

static void test_ok(void) {
  printf("PASS\n");
  tests_passed++;
}

static void test_fail(const char *reason) { printf("FAIL (%s)\n", reason); }

static void test_i_new_valid(void) {
  test_start("index", "i_new valid args returns non-NULL with correct fields");
  time_t t = 12345;
  struct Index *idx = i_new("foo.txt", "abc123def", t);
  if (!idx) {
    test_fail("i_new returned NULL");
    return;
  }
  if (idx->change_time != t) {
    i_free(idx);
    test_fail("change_time mismatch");
    return;
  }
  if (strcmp(idx->file_name, "foo.txt") != 0) {
    i_free(idx);
    test_fail("file_name mismatch");
    return;
  }
  if (strcmp(idx->hash, "abc123def") != 0) {
    i_free(idx);
    test_fail("hash mismatch");
    return;
  }
  i_free(idx);
  test_ok();
}

static void test_i_new_filename_too_long(void) {
  test_start("index", "i_new filename longer than FILENAME_MAX returns NULL");
  char *long_name = malloc(FILENAME_MAX + 2);
  if (!long_name) {
    test_fail("malloc failed");
    return;
  }
  memset(long_name, 'a', FILENAME_MAX + 1);
  long_name[FILENAME_MAX + 1] = '\0';
  struct Index *idx = i_new(long_name, "abc", (time_t)0);
  free(long_name);
  if (idx != NULL) {
    i_free(idx);
    test_fail("i_new should return NULL for too-long filename");
    return;
  }
  test_ok();
}

static void test_i_new_hash_too_long(void) {
  test_start("index", "i_new hash longer than 64 returns NULL");
  char long_hash[66];
  memset(long_hash, 'a', 65);
  long_hash[65] = '\0';
  struct Index *idx = i_new("x", long_hash, (time_t)0);
  if (idx != NULL) {
    i_free(idx);
    test_fail("i_new should return NULL for too-long hash");
    return;
  }
  test_ok();
}

static void test_i_free_no_crash(void) {
  test_start("index", "i_free after i_new does not crash");
  struct Index *idx = i_new("a", "b", (time_t)0);
  if (!idx) {
    test_fail("i_new failed");
    return;
  }
  i_free(idx);
  test_ok();
}

static void test_i_load_null(void) {
  test_start("index", "i_load NULL returns NULL");
  struct Index *idx = i_load(NULL);
  if (idx != NULL) {
    i_free(idx);
    test_fail("i_load(NULL) should return NULL");
    return;
  }
  test_ok();
}

static void test_i_load_valid(void) {
  test_start("index", "i_load valid string returns correct Index");
  const char *s = "99999 myfile.c a1b2c3d4e5f6";
  struct Index *idx = i_load(s);
  if (!idx) {
    test_fail("i_load returned NULL");
    return;
  }
  if (idx->change_time != (time_t)99999) {
    i_free(idx);
    test_fail("change_time mismatch");
    return;
  }
  if (strcmp(idx->file_name, "myfile.c") != 0) {
    i_free(idx);
    test_fail("file_name mismatch");
    return;
  }
  if (strcmp(idx->hash, "a1b2c3d4e5f6") != 0) {
    i_free(idx);
    test_fail("hash mismatch");
    return;
  }
  i_free(idx);
  test_ok();
}

static void test_i_load_malformed(void) {
  test_start("index", "i_load malformed string returns NULL");
  struct Index *idx = i_load("only two tokens");
  if (idx != NULL) {
    i_free(idx);
    test_fail("i_load malformed should return NULL");
    return;
  }
  idx = i_load("");
  if (idx != NULL) {
    i_free(idx);
    test_fail("i_load empty should return NULL");
    return;
  }
  test_ok();
}

static void test_get_null_ht(void) {
  test_start("index", "get NULL ht returns NULL");
  const struct Index *p = i_get(NULL, "key");
  if (p != NULL) {
    test_fail("get(NULL, key) should return NULL");
    return;
  }
  test_ok();
}

static void test_get_null_key(void) {
  test_start("index", "get ht with NULL key returns NULL");
  struct HashTable *ht = ht_new(HT_MAX_SIZE);
  if (!ht) {
    test_fail("ht_new failed");
    return;
  }
  const struct Index *p = i_get(ht, NULL);
  ht_free(ht);
  if (p != NULL) {
    test_fail("get(ht, NULL) should return NULL");
    return;
  }
  test_ok();
}

static void test_get_existing_key(void) {
  test_start("index", "get existing key returns correct Index");
  struct HashTable *ht = ht_new(HT_MAX_SIZE);
  if (!ht) {
    test_fail("ht_new failed");
    return;
  }
  struct Index *idx = i_new("path/to/file", "deadbeef", (time_t)42);
  if (!idx) {
    ht_free(ht);
    test_fail("i_new failed");
    return;
  }
  if (ht_add(ht, "path/to/file", idx) != HT_OK) {
    i_free(idx);
    ht_free(ht);
    test_fail("ht_add failed");
    return;
  }
  const struct Index *got = i_get(ht, "path/to/file");
  if (!got || got != idx) {
    ht_free(ht);
    test_fail("get should return the added Index");
    return;
  }
  if (strcmp(got->file_name, "path/to/file") != 0 ||
      strcmp(got->hash, "deadbeef") != 0 || got->change_time != (time_t)42) {
    ht_free(ht);
    test_fail("get returned wrong Index fields");
    return;
  }
  ht_free(ht);
  test_ok();
}

static void test_get_missing_key(void) {
  test_start("index", "get missing key returns NULL");
  struct HashTable *ht = ht_new(HT_MAX_SIZE);
  if (!ht) {
    test_fail("ht_new failed");
    return;
  }
  const struct Index *p = i_get(ht, "nonexistent");
  ht_free(ht);
  if (p != NULL) {
    test_fail("get missing key should return NULL");
    return;
  }
  test_ok();
}

static void test_load_no_file(void) {
  test_start("index", "load when index file missing returns NULL");
  char cwd[1024];
  if (!getcwd(cwd, sizeof(cwd))) {
    test_fail("getcwd failed");
    return;
  }
  if (chdir("/tmp") != 0) {
    test_fail("chdir /tmp failed");
    return;
  }
  struct HashTable *ht = im_load();
  if (chdir(cwd) != 0) {
    if (ht)
      ht_free(ht);
    test_fail("chdir back failed");
    return;
  }
  if (ht != NULL) {
    ht_free(ht);
    test_fail("load() should return NULL when .berero/index does not exist");
    return;
  }
  test_ok();
}

int main(void) {
  printf("\n=== index.c tests ===\n\n");

  test_i_new_valid();
  test_i_new_filename_too_long();
  test_i_new_hash_too_long();
  test_i_free_no_crash();

  test_i_load_null();
  test_i_load_valid();
  test_i_load_malformed();

  test_get_null_ht();
  test_get_null_key();
  test_get_existing_key();
  test_get_missing_key();

  test_load_no_file();

  printf("\n--- Summary ---\n");
  printf("  Passed: %d / %d\n", tests_passed, tests_run);
  return tests_passed == tests_run ? 0 : 1;
}
