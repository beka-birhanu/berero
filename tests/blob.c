/*
 * Tests for utiles/blob.c (chimek, zerga, bwrite, bread).
 * File-based mapping: utiles/blob.c <-> tests/blob.c
 */

#include "../utiles/blob.h"
#include "../sub_commands/init.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

#define CHUNK_SIZE 16384

static int tests_run;
static int tests_passed;

static void test_start(const char *fn, const char *name) {
  printf("  [%s] %s ... ", fn, name);
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

/* --- chimek --- */

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
  if (ret != BLOB_OK) {
    fclose(dst);
    test_fail("chimek returned non-BLOB_OK");
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
  if (ret != BLOB_OK) {
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
  test_start("chimek", "small data round-trip");
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
  if (ret != BLOB_OK) {
    fclose(compressed);
    test_fail("chimek returned non-BLOB_OK");
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
  if (ret != BLOB_OK) {
    fclose(decompressed);
    test_fail("zerga returned non-BLOB_OK");
    return;
  }
  rewind(decompressed);
  char *buf = malloc(len + 1);
  if (!buf) {
    fclose(decompressed);
    test_fail("malloc failed");
    return;
  }
  size_t nr = fread(buf, 1, len + 1, decompressed);
  fclose(decompressed);
  if (nr != len || memcmp(buf, content, len) != 0) {
    free(buf);
    test_fail("decompressed content does not match original");
    return;
  }
  free(buf);
  test_ok();
}

static void test_chimek_larger_than_chunk(void) {
  test_start("chimek", "data larger than CHUNK_SIZE round-trips");
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
  if (ret != BLOB_OK) {
    fclose(compressed);
    test_fail("chimek returned non-BLOB_OK");
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
  if (ret != BLOB_OK) {
    fclose(decompressed);
    test_fail("zerga returned non-BLOB_OK");
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

/* --- zerga --- */

static void test_zerga_empty_input(void) {
  test_start("zerga", "empty input returns error");
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
  if (ret == BLOB_OK) {
    test_fail("zerga on empty input should not return BLOB_OK");
    return;
  }
  test_ok();
}

static void test_zerga_corrupt_data(void) {
  test_start("zerga", "corrupt gzip data returns error");
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
  if (ret == BLOB_OK) {
    test_fail("zerga on corrupt data should not return BLOB_OK");
    return;
  }
  test_ok();
}

static void test_zerga_partial_gzip_header(void) {
  test_start("zerga", "truncated gzip header returns error");
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
  if (ret == BLOB_OK) {
    test_fail("zerga on truncated data should not return BLOB_OK");
    return;
  }
  test_ok();
}

/* --- bwrite / bread --- */

static void test_bwrite_short_dest(void) {
  test_start("bwrite",
             "dest shorter than BLOB_MIN_FILE_NAME returns BLOB_ERROR");
  if (bwrite("/nonexistent", "ab") != BLOB_ERROR) {
    test_fail("expected BLOB_ERROR");
    return;
  }
  test_ok();
}

static void test_bread_short_source(void) {
  test_start("bread",
             "source shorter than BLOB_MIN_FILE_NAME returns BLOB_ERROR");
  if (bread("ab", "/tmp/out") != BLOB_ERROR) {
    test_fail("expected BLOB_ERROR");
    return;
  }
  test_ok();
}

static void test_bwrite_roundtrip(void) {
  test_start("bwrite", "bwrite then bread round-trip");
  char cwd[PATH_MAX];
  if (!getcwd(cwd, sizeof(cwd))) {
    test_fail("getcwd failed");
    return;
  }
  char tmpdir[] = "/tmp/berero_blob_XXXXXX";
  if (!mkdtemp(tmpdir)) {
    test_fail("mkdtemp failed");
    return;
  }
  if (chdir(tmpdir) != 0) {
    test_fail("chdir to tmp failed");
    return;
  }
  if (mkdir(INIT_ENTRY_DIR, 0700) != 0 || mkdir(INIT_OBJECT_DIR, 0700) != 0) {
    test_fail("mkdir .berero/objects failed");
    chdir(cwd);
    return;
  }
  const char *content = "hello blob\n";
  size_t len = strlen(content);
  FILE *f = fopen("src.txt", "wb");
  if (!f) {
    test_fail("create src.txt failed");
    chdir(cwd);
    return;
  }
  if (fwrite(content, 1, len, f) != len) {
    fclose(f);
    chdir(cwd);
    test_fail("write src.txt failed");
    return;
  }
  fclose(f);
  if (bwrite("src.txt", "abc") != BLOB_OK) {
    chdir(cwd);
    test_fail("bwrite failed");
    return;
  }
  chdir(cwd);
  test_ok();
}

int main(void) {
  printf("\n=== blob.c tests ===\n\n");
  printf("--- chimek ---\n");
  test_chimek_empty_source();
  test_chimek_small_data();
  test_chimek_larger_than_chunk();
  printf("--- zerga ---\n");
  test_zerga_empty_input();
  test_zerga_corrupt_data();
  test_zerga_partial_gzip_header();
  printf("--- bwrite / bread ---\n");
  test_bwrite_short_dest();
  test_bread_short_source();
  test_bwrite_roundtrip();
  printf("\n  Passed: %d / %d\n", tests_passed, tests_run);
  return tests_passed == tests_run ? 0 : 1;
}
