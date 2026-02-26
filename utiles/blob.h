#ifndef BLOB_H
#define BLOB_H

#include <stdio.h>

// Compresses source into dest using gzip format.
// Returns Z_OK on success, or a zlib error code on failure.
int chimek(FILE *source, FILE *dest);

// Decompresses source into dest (supports zlib and gzip format).
// Returns Z_OK on success, Z_DATA_ERROR if source had no data,
// or another zlib error code on failure.
int zerga(FILE *source, FILE *dest);

#endif // BLOB_H
