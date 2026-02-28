#ifndef BLOB_H
#define BLOB_H

#include <stdio.h>

#define BLOB_OK 0
#define BLOB_ERROR -1
#define BLOB_MIN_FILE_NAME 3

// Compresses the file at source and writes the blob to dest.
// Returns BLOB_OK on success, or BLOB_ERROR on failure.
//
// Note: The blob may not be stored at th original filename(dest); bread will
//       return the same data when given the dest as source.
int bwrite(const char *source, const char *dest);

// Decompresses the blob at source and writes the file to dest.
// Returns BLOB_OK on success, or BLOB_ERROR on failure.
//
// Note: source must be a blob path given to bwrite as source.
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

#endif // BLOB_H
