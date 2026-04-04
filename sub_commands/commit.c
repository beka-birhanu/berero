#include "commit.h"
#include "../utiles/blob.h"
#include "../utiles/hash.h"
#include "../utiles/index.h"
#include "../utiles/tree.h"
#include "help.h"
#include <errno.h>
#include <openssl/evp.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <zlib.h>

int _hash_commit(const char *tfat, const char *tfategna,
                 const unsigned char *tree_hash, unsigned char *out);
void _get_timezone(char *buf, size_t len);

int commit(int argc, char *argv[]) {
  if (argc != 3) {
    help(argc, argv);
    return COMMIT_ERROR;
  }

  const char *tfat = argv[1];
  const char *tfategna = argv[2];

  struct INode *i = i_load();
  if (!i) {
    return COMMIT_ERROR;
  }

  if (i->n_daughters == 0) {
    i_free(i);
    return COMMIT_ERROR;
  }

  if (dump_tree(i, "") != COMMIT_OK) {
    i_free(i);
    return COMMIT_ERROR;
  }

  unsigned char *hash_bin = malloc(HASH_LEN);
  if (!hash_bin) {
    i_free(i);
    return COMMIT_ERROR;
  }

  if (_hash_commit(tfat, tfategna, i->hash, hash_bin) != HASH_OK) {
    free(hash_bin);
    i_free(i);
    return COMMIT_ERROR;
  }

  char hash[HASH_LEN * 2 + 1];
  sh_bin_to_hex(hash_bin, HASH_LEN, hash);

  char dir_out[BLOB_OBJECT_DIR_LEN];
  if (object_file_dir_location(hash, dir_out) != COMMIT_OK) {
    free(hash_bin);
    i_free(i);
    return COMMIT_ERROR;
  }

  if (mkdir(dir_out, BLOB_DIR_PRIVATE_MODE) == -1 && errno != EEXIST) {
    perror(dir_out);
    free(hash_bin);
    i_free(i);
    return COMMIT_ERROR;
  }

  char file_out[BLOB_OBJECT_FILE_LEN];
  if (object_file_location(hash, file_out) != COMMIT_OK) {
    free(hash_bin);
    i_free(i);
    return COMMIT_ERROR;
  }

  FILE *f = fopen(file_out, "wb");
  if (!f) {
    perror(file_out);
    free(hash_bin);
    i_free(i);
    return COMMIT_ERROR;
  }

  char i_hash[HASH_LEN * 2 + 1];
  sh_bin_to_hex(i->hash, HASH_LEN, i_hash);

  time_t now = time(NULL);
  char tz[6];
  _get_timezone(tz, sizeof(tz));

  if (fprintf(f, "ts %lu %s\n", now, tz) < 0) {
    fclose(f);
    free(hash_bin);
    i_free(i);
    return COMMIT_ERROR;
  }

  if (fprintf(f, "zirzir %s\n", i_hash) < 0) {
    fclose(f);
    free(hash_bin);
    i_free(i);
    return COMMIT_ERROR;
  }

  if (access(COMMIT_HEAD_FILE, F_OK) == 0) {
    FILE *head = fopen(COMMIT_HEAD_FILE, "rb");
    if (!head) {
      free(hash_bin);
      i_free(i);
      return COMMIT_ERROR;
    }
    char *head_hash = malloc(HASH_LEN * 2 + 1);
    if (!head_hash) {
      free(hash_bin);
      i_free(i);
      return COMMIT_ERROR;
    }
    if (fscanf(head, "%s", head_hash) != 1) {
      free(hash_bin);
      i_free(i);
      return COMMIT_ERROR;
    }

    if (fprintf(f, "parent %s\n", head_hash) < 0) {
      perror(COMMIT_HEAD_FILE);
      free(hash_bin);
      i_free(i);
      return COMMIT_ERROR;
    }
    fclose(head);
  }

  if (fprintf(f, "tfategna %s\n", tfategna) < 0) {
    fclose(f);
    free(hash_bin);
    i_free(i);
    return COMMIT_ERROR;
  }

  if (fprintf(f, "tfat %s\n", tfat) < 0) {
    fclose(f);
    free(hash_bin);
    i_free(i);
    return COMMIT_ERROR;
  }

  if (fclose(f) != 0) {
    free(hash_bin);
    i_free(i);
    return COMMIT_ERROR;
  }

  f = fopen(COMMIT_HEAD_FILE, "wb");
  if (!f) {
    perror(COMMIT_HEAD_FILE);
    free(hash_bin);
    i_free(i);
    return COMMIT_ERROR;
  }

  if (fprintf(f, "%s\n", hash) < 0) {
    fclose(f);
    free(hash_bin);
    i_free(i);
    return COMMIT_ERROR;
  }

  if (fclose(f) != 0) {
    free(hash_bin);
    i_free(i);
    return COMMIT_ERROR;
  }

  printf("commited\n");
  printf("hash: %s\n", hash);

  free(hash_bin);
  i_free(i);
  return COMMIT_OK;
}

int _hash_commit(const char *tfat, const char *tfategna,
                 const unsigned char *tree_hash, unsigned char *out) {
  if (!tree_hash || !tfategna || !tfat || !out)
    return HASH_ERROR;

  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx)
    return HASH_ERROR;

  if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1) {
    EVP_MD_CTX_free(ctx);
    return HASH_ERROR;
  }

  EVP_DigestUpdate(ctx, tree_hash, HASH_LEN);
  const char sep = '\n';
  EVP_DigestUpdate(ctx, &sep, 1);
  EVP_DigestUpdate(ctx, tfat, strlen(tfat));
  EVP_DigestUpdate(ctx, &sep, 1);
  EVP_DigestUpdate(ctx, tfategna, strlen(tfategna));

  unsigned int len;
  if (EVP_DigestFinal_ex(ctx, out, &len) != 1 || len != HASH_LEN) {
    EVP_MD_CTX_free(ctx);
    return HASH_ERROR;
  }

  EVP_MD_CTX_free(ctx);
  return HASH_OK;
}

void _get_timezone(char *buf, size_t len) {
  time_t now = time(NULL);

  struct tm local = *localtime(&now);
  struct tm gm = *gmtime(&now);

  time_t local_t = mktime(&local);
  time_t gm_t = mktime(&gm);

  int diff = (int)difftime(local_t, gm_t);

  int hours = diff / 3600;
  int mins = (diff % 3600) / 60;

  // Format +HHMM
  snprintf(buf, len, "%+03d%02d", hours, mins < 0 ? -mins : mins);
}
