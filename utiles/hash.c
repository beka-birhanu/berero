#include "hash.h"
#include <openssl/evp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK_SIZE 16384
#define EVP_OK 1
#define ENTRY_LEN (HASH_LEN + sizeof(uint16_t) + sizeof(uint16_t))

static int cmp_hashes(const void *a, const void *b) {
  const unsigned char *ha = *(const unsigned char **)a;
  const unsigned char *hb = *(const unsigned char **)b;
  return memcmp(ha, hb, ENTRY_LEN);
}

void sh_bin_to_hex(const unsigned char *hash, unsigned int len, char *out) {
  static const char hex_chars[] = "0123456789abcdef";

  for (unsigned int i = 0; i < len; i++) {
    out[i * 2] = hex_chars[(hash[i] >> 4) & 0xF];
    out[i * 2 + 1] = hex_chars[hash[i] & 0xF];
  }
  out[len * 2] = '\0';
}

int sh_hash(FILE *file, unsigned char *hash) {
  size_t read_in;
  int evp_ret;

  if (!file || !hash)
    return HASH_ERROR;

  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx)
    return HASH_ERROR;

  if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1)
    return HASH_ERROR;

  unsigned char in[CHUNK_SIZE];

  for (;;) {
    read_in = fread(in, 1, CHUNK_SIZE, file);
    if (ferror(file)) {
      EVP_MD_CTX_free(ctx);
      return HASH_ERROR;
    }
    if (read_in == 0)
      break;

    evp_ret = EVP_DigestUpdate(ctx, in, read_in);
    if (evp_ret != EVP_OK) {
      EVP_MD_CTX_free(ctx);
      return HASH_ERROR;
    }
  }

  unsigned int len;
  if ((evp_ret = EVP_DigestFinal_ex(ctx, hash, &len)) != EVP_OK) {
    EVP_MD_CTX_free(ctx);
    return HASH_ERROR;
  }

  EVP_MD_CTX_free(ctx);
  return HASH_OK;
}

int sh_combine_hash(const unsigned char **hashes, size_t count,
                    unsigned char *out) {
  if (!hashes || !out)
    return HASH_ERROR;

  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx)
    return HASH_ERROR;

  if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1) {
    EVP_MD_CTX_free(ctx);
    return HASH_ERROR;
  }

  /* make order irrelevant */
  qsort(hashes, count, sizeof(unsigned char *), cmp_hashes);

  for (size_t i = 0; i < count; i++) {
    if (EVP_DigestUpdate(ctx, hashes[i], ENTRY_LEN) != 1) {
      EVP_MD_CTX_free(ctx);
      return HASH_ERROR;
    }
  }

  unsigned int len;
  if (EVP_DigestFinal_ex(ctx, out, &len) != 1 || len != HASH_LEN) {
    EVP_MD_CTX_free(ctx);
    return HASH_ERROR;
  }

  EVP_MD_CTX_free(ctx);
  return HASH_OK;
}
