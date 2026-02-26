#include "hash.h"
#include <openssl/evp.h>
#include <stdio.h>
#include <stdlib.h>

#define CHUNK_SIZE 16384
#define EVP_OK 1

int hash(FILE *file, unsigned char *hash) {
  size_t read_in;
  int evp_ret;

  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx)
    return -1;

  if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1)
    return -1;

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
