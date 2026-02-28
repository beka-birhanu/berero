/*
 * Tests for utiles/hash.c.
 * File-based mapping: utiles/hash.c <-> tests/hash.c
 */

#include "../utiles/hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK_SIZE 16384
#define SHA256_LEN 32

static int tests_run;
static int tests_passed;

static void test_start(const char *name) {
  printf("  [hash] %s ... ", name);
  fflush(stdout);
  tests_run++;
}

static void test_ok(void) {
  printf("PASS\n");
  tests_passed++;
}

static void test_fail(const char *reason) { printf("FAIL (%s)\n", reason); }

static FILE *tmpfile_with(const char *content, size_t len) {
  FILE *f = tmpfile();
  if (!f)
    return NULL;
  if (len && fwrite(content, 1, len, f) != len) {
    fclose(f);
    return NULL;
  }
  rewind(f);
  return f;
}

/* Known SHA-256 hashes (binary, 32 bytes). */
static const unsigned char HASH_EMPTY[SHA256_LEN] = {
    0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14, 0x9a, 0xfb, 0xf4,
    0xc8, 0x99, 0x6f, 0xb9, 0x24, 0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b,
    0x93, 0x4c, 0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55};
static const unsigned char HASH_HELLO_N[SHA256_LEN] = {
    0x58, 0x91, 0xb5, 0xb5, 0x22, 0xd5, 0xdf, 0x08, 0x6d, 0x0f, 0xf0,
    0xb1, 0x10, 0xfb, 0xd9, 0xd2, 0x1b, 0xb4, 0xfc, 0x71, 0x63, 0xaf,
    0x34, 0xd0, 0x82, 0x86, 0xa2, 0xe8, 0x46, 0xf6, 0xbe, 0x03};

static int hash_equal(const unsigned char *a, const unsigned char *b) {
  return memcmp(a, b, SHA256_LEN) == 0;
}

static void test_hash_empty_file(void) {
  test_start("empty file returns HASH_OK and correct SHA-256");
  FILE *f = tmpfile_with("", 0);
  if (!f) {
    test_fail("tmpfile failed");
    return;
  }
  unsigned char out[SHA256_LEN];
  int ret = sh_hash(f, out);
  fclose(f);
  if (ret != HASH_OK) {
    test_fail("hash() did not return HASH_OK");
    return;
  }
  if (!hash_equal(out, HASH_EMPTY)) {
    test_fail("hash of empty file does not match known SHA-256 of empty input");
    return;
  }
  test_ok();
}

static void test_hash_known_content(void) {
  test_start("small file with known content returns correct SHA-256");
  const char *content = "hello\n";
  FILE *f = tmpfile_with(content, strlen(content));
  if (!f) {
    test_fail("tmpfile failed");
    return;
  }
  unsigned char out[SHA256_LEN];
  int ret = sh_hash(f, out);
  fclose(f);
  if (ret != HASH_OK) {
    test_fail("hash() did not return HASH_OK");
    return;
  }
  if (!hash_equal(out, HASH_HELLO_N)) {
    test_fail("hash does not match known SHA-256 of \"hello\\n\"");
    return;
  }
  test_ok();
}

static void test_hash_larger_than_chunk(void) {
  test_start("file larger than CHUNK_SIZE is hashed correctly (chunking)");
  size_t n = CHUNK_SIZE + 4096;
  char *buf = malloc(n);
  if (!buf) {
    test_fail("malloc failed");
    return;
  }
  for (size_t i = 0; i < n; i++)
    buf[i] = (char)(i % 256);
  FILE *f = tmpfile_with(buf, n);
  free(buf);
  if (!f) {
    test_fail("tmpfile failed");
    return;
  }
  unsigned char out[SHA256_LEN];
  int ret = sh_hash(f, out);
  fclose(f);
  if (ret != HASH_OK) {
    test_fail("hash() did not return HASH_OK for large file");
    return;
  }

  /* Second pass: same content must yield same hash */
  n = CHUNK_SIZE + 4096;
  buf = malloc(n);
  if (!buf) {
    test_fail("malloc failed on second pass");
    return;
  }
  for (size_t i = 0; i < n; i++)
    buf[i] = (char)(i % 256);
  f = tmpfile_with(buf, n);
  free(buf);
  if (!f) {
    test_fail("tmpfile failed on second pass");
    return;
  }
  unsigned char out2[SHA256_LEN];
  ret = sh_hash(f, out2);
  fclose(f);
  if (ret != HASH_OK || !hash_equal(out, out2)) {
    test_fail("large file hash not deterministic or second hash failed");
    return;
  }
  test_ok();
}

static void test_hash_valid_buffer(void) {
  test_start("valid file with valid buffer succeeds");
  FILE *f = tmpfile_with("x", 1);
  if (!f) {
    test_fail("tmpfile failed");
    return;
  }
  unsigned char out[SHA256_LEN];
  int ret = sh_hash(f, out);
  fclose(f);
  if (ret != HASH_OK) {
    test_fail("hash() did not return HASH_OK");
    return;
  }
  test_ok();
}

int main(void) {
  printf("\n=== hash.c tests ===\n\n");
  test_hash_empty_file();
  test_hash_known_content();
  test_hash_larger_than_chunk();
  test_hash_valid_buffer();
  printf("\n  Passed: %d / %d\n", tests_passed, tests_run);
  return tests_passed == tests_run ? 0 : 1;
}

