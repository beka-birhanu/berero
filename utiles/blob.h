#ifndef BLOB_H
#define BLOB_H

#include "../sub_commands/init.h"
#include "hash.h"
#include <stdio.h>

#define BLOB_OK 0
#define BLOB_ERROR -1
#define BLOB_MIN_FILE_NAME 3

#define BLOB_BASE_OBJECT_DIR_LEN (sizeof(INIT_OBJECT_DIR) - 1)
// base + '/' + 2 chars + '\0'
#define BLOB_OBJECT_DIR_LEN (BLOB_BASE_OBJECT_DIR_LEN + 1 + 2 + 1)
// base + '/' + 2 + '/' + rest + '\0'
#define BLOB_OBJECT_FILE_LEN                                                   \
  (BLOB_BASE_OBJECT_DIR_LEN + 1 + 2 + 1 + (HASH_LEN * 2 - 2) + 1)

#define BLOB_DIR_PRIVATE_MODE 0700

// Compresses the file at source and writes the blob to dest.
// Returns BLOB_OK on success, or BLOB_ERROR on failure.
//
// Note: The blob will be stored in the location given by the rutine
//       "object_file_location".
int bwrite(const char *source, const char *dest);

// Decompresses the blob at source and writes the file to dest.
// Returns BLOB_OK on success, or BLOB_ERROR on failure.
//
// Note: source must be a blob path given to bwrite as source. since the read
//       location will be the out put of rutine "object_file_location".
int bread(const char *source, const char *dest);

// Compresses source into dest using gzip format.
// Returns BLOB_OK on success, BLOB_ERROR on failure.
//
// Note: it is assumed that both source and dest are already opened.
int chimek(FILE *source, FILE *dest);

// Decompresses source into dest (supports zlib and gzip format).
// Returns BLOB_OK on success, BLOB_ERROR on failure.
//
// Note: it is assumed that both source and dest are already opened.
int zerga(FILE *source, FILE *dest);

// Returns the location of the blob file for the given hash.
int object_file_location(const char *hash, char *file_out);

// Returns the location of the directory the blob file belongs to.
int object_file_dir_location(const char *hash, char *dir_out);

#endif // BLOB_H
