/*
 * Test suite for utiles: hash, chimek (compress), zerga (decompress).
 * Runs all edge-case tests and prints descriptive PASS/FAIL output.
 */

#include "../utiles/blob.h"
#include "../utiles/hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define CHUNK_SIZE 16384
#define SHA256_LEN 32

/* --- Helpers --- */

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

/* Write content to a temp file and rewind. Caller fcloses. */
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

/* --- hash() tests --- */

static void test_hash_empty_file(void) {
  test_start("hash", "empty file returns HASH_OK and correct SHA-256");
  FILE *f = tmpfile_with("", 0);
  if (!f) {
    test_fail("tmpfile failed");
    return;
  }
  unsigned char out[SHA256_LEN];
  int ret = hash(f, out);
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
  test_start("hash", "small file with known content returns correct SHA-256");
  const char *content = "hello\n";
  FILE *f = tmpfile_with(content, strlen(content));
  if (!f) {
    test_fail("tmpfile failed");
    return;
  }
  unsigned char out[SHA256_LEN];
  int ret = hash(f, out);
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
  test_start("hash",
             "file larger than CHUNK_SIZE is hashed correctly (chunking)");
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
  int ret = hash(f, out);
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
  ret = hash(f, out2);
  fclose(f);
  if (ret != HASH_OK || !hash_equal(out, out2)) {
    test_fail("large file hash not deterministic or second hash failed");
    return;
  }
  test_ok();
}

static void test_hash_null_buffer_undefined(void) {
  /* Documented: we only test with valid buffer; NULL hash is caller bug. */
  test_start("hash", "valid file with valid buffer succeeds");
  FILE *f = tmpfile_with("x", 1);
  if (!f) {
    test_fail("tmpfile failed");
    return;
  }
  unsigned char out[SHA256_LEN];
  int ret = hash(f, out);
  fclose(f);
  if (ret != HASH_OK) {
    test_fail("hash() did not return HASH_OK");
    return;
  }
  test_ok();
}

/* --- chimek() / zerga() tests --- */

static void test_chimek_empty_source(void) {
  test_start("chimek", "empty source produces valid gzip stream");
  FILE *src = tmpfile_with("", 0);
  if (!src) {
    test_fail("tmpfile failed");
    return;
  }
  FILE *dst = tmpfile();
  if (!dst) {
    fclose(src);
    test_fail("tmpfile for dest failed");
    return;
  }
  int ret = chimek(src, dst);
  fclose(src);
  if (ret != Z_OK) {
    fclose(dst);
    test_fail("chimek returned non-Z_OK");
    return;
  }
  rewind(dst);
  FILE *back = tmpfile();
  if (!back) {
    fclose(dst);
    test_fail("tmpfile for round-trip failed");
    return;
  }
  ret = zerga(dst, back);
  fclose(dst);
  if (ret != Z_OK) {
    fclose(back);
    test_fail("zerga of empty gzip stream failed");
    return;
  }
  rewind(back);
  char buf[1];
  size_t n = fread(buf, 1, 1, back);
  fclose(back);
  if (n != 0) {
    test_fail("decompressed empty stream should be empty");
    return;
  }
  test_ok();
}

static void test_chimek_small_data(void) {
  test_start("chimek",
             "small data round-trip (compress then decompress matches)");
  const char *content = "hello world\n";
  size_t len = strlen(content);
  FILE *src = tmpfile_with(content, len);
  if (!src) {
    test_fail("tmpfile failed");
    return;
  }
  FILE *compressed = tmpfile();
  if (!compressed) {
    fclose(src);
    test_fail("tmpfile for compressed failed");
    return;
  }
  int ret = chimek(src, compressed);
  fclose(src);
  if (ret != Z_OK) {
    fclose(compressed);
    test_fail("chimek returned non-Z_OK");
    return;
  }
  rewind(compressed);
  FILE *decompressed = tmpfile();
  if (!decompressed) {
    fclose(compressed);
    test_fail("tmpfile for decompressed failed");
    return;
  }
  ret = zerga(compressed, decompressed);
  fclose(compressed);
  if (ret != Z_OK) {
    fclose(decompressed);
    test_fail("zerga returned non-Z_OK");
    return;
  }
  rewind(decompressed);
  char *buf = malloc(len + 1);
  if (!buf) {
    fclose(decompressed);
    test_fail("malloc failed");
    return;
  }
  size_t n = fread(buf, 1, len + 1, decompressed);
  fclose(decompressed);
  if (n != len || memcmp(buf, content, len) != 0) {
    free(buf);
    test_fail("decompressed content does not match original");
    return;
  }
  free(buf);
  test_ok();
}

static void test_chimek_larger_than_chunk(void) {
  test_start("chimek", "data larger than CHUNK_SIZE round-trips correctly");
  size_t n = CHUNK_SIZE + 1024;
  char *content = malloc(n);
  if (!content) {
    test_fail("malloc failed");
    return;
  }
  for (size_t i = 0; i < n; i++)
    content[i] = (char)(i % 256);
  FILE *src = tmpfile_with(content, n);
  free(content);
  if (!src) {
    test_fail("tmpfile failed");
    return;
  }
  FILE *compressed = tmpfile();
  if (!compressed) {
    fclose(src);
    test_fail("tmpfile for compressed failed");
    return;
  }
  int ret = chimek(src, compressed);
  fclose(src);
  if (ret != Z_OK) {
    fclose(compressed);
    test_fail("chimek returned non-Z_OK");
    return;
  }
  rewind(compressed);
  FILE *decompressed = tmpfile();
  if (!decompressed) {
    fclose(compressed);
    test_fail("tmpfile for decompressed failed");
    return;
  }
  ret = zerga(compressed, decompressed);
  fclose(compressed);
  if (ret != Z_OK) {
    fclose(decompressed);
    test_fail("zerga returned non-Z_OK");
    return;
  }
  rewind(decompressed);
  char *buf = malloc(n);
  if (!buf) {
    fclose(decompressed);
    test_fail("malloc for read failed");
    return;
  }
  size_t nr = fread(buf, 1, n, decompressed);
  fclose(decompressed);
  if (nr != n) {
    free(buf);
    test_fail("decompressed size does not match original");
    return;
  }
  n = CHUNK_SIZE + 1024;
  content = malloc(n);
  if (!content) {
    free(buf);
    test_fail("malloc for compare failed");
    return;
  }
  for (size_t i = 0; i < n; i++)
    content[i] = (char)(i % 256);
  if (memcmp(buf, content, n) != 0) {
    free(buf);
    free(content);
    test_fail("decompressed content does not match original");
    return;
  }
  free(buf);
  free(content);
  test_ok();
}

static void test_zerga_empty_input(void) {
  test_start("zerga", "empty input returns error (no valid gzip stream)");
  FILE *src = tmpfile_with("", 0);
  if (!src) {
    test_fail("tmpfile failed");
    return;
  }
  FILE *dst = tmpfile();
  if (!dst) {
    fclose(src);
    test_fail("tmpfile for dest failed");
    return;
  }
  int ret = zerga(src, dst);
  fclose(src);
  fclose(dst);
  if (ret == Z_OK) {
    test_fail("zerga on empty input should not return Z_OK");
    return;
  }
  test_ok();
}

static void test_zerga_corrupt_data(void) {
  test_start("zerga", "corrupt/invalid gzip data returns error");
  const char *corrupt = "not gzip data at all";
  FILE *src = tmpfile_with(corrupt, strlen(corrupt));
  if (!src) {
    test_fail("tmpfile failed");
    return;
  }
  FILE *dst = tmpfile();
  if (!dst) {
    fclose(src);
    test_fail("tmpfile for dest failed");
    return;
  }
  int ret = zerga(src, dst);
  fclose(src);
  fclose(dst);
  if (ret == Z_OK) {
    test_fail("zerga on corrupt data should not return Z_OK");
    return;
  }
  test_ok();
}

static void test_zerga_partial_gzip_header(void) {
  test_start("zerga", "truncated gzip header returns error");
  /* Minimal invalid: just a few bytes that look like start of gzip */
  unsigned char partial[] = {0x1f, 0x8b, 0x08, 0x00};
  FILE *src = tmpfile_with((const char *)partial, sizeof(partial));
  if (!src) {
    test_fail("tmpfile failed");
    return;
  }
  FILE *dst = tmpfile();
  if (!dst) {
    fclose(src);
    test_fail("tmpfile for dest failed");
    return;
  }
  int ret = zerga(src, dst);
  fclose(src);
  fclose(dst);
  if (ret == Z_OK) {
    test_fail("zerga on truncated data should not return Z_OK");
    return;
  }
  test_ok();
}

int main(void) {
  printf("\n=== Utiles test suite (hash, chimek, zerga) ===\n\n");

  printf("--- hash() ---\n");
  test_hash_empty_file();
  test_hash_known_content();
  test_hash_larger_than_chunk();
  test_hash_null_buffer_undefined();

  printf("--- chimek() (compress) ---\n");
  test_chimek_empty_source();
  test_chimek_small_data();
  test_chimek_larger_than_chunk();

  printf("--- zerga() (decompress) ---\n");
  test_zerga_empty_input();
  test_zerga_corrupt_data();
  test_zerga_partial_gzip_header();

  printf("\n--- Summary ---\n");
  printf("  Passed: %d / %d\n", tests_passed, tests_run);
  if (tests_passed == tests_run)
    printf("  Result: ALL TESTS PASSED\n\n");
  else
    printf("  Result: SOME TESTS FAILED\n\n");

  return tests_passed == tests_run ? 0 : 1;
}
