#ifndef HASH_H
#define HASH_H

#include <stdio.h>

#define HASH_OK 0
#define HASH_ERROR -1
#define HASH_LEN 32 // SHA256 size

// Computes the SHA-256 of the file passed.
// Returns Hash_OK on success, Hash_ERROR on failure.
int sh_hash(FILE *file, unsigned char *hash);

// Combines the hashes of the files passed into a single hash after sorting to
// ensure order doesn't matter. Returns Hash_OK on success, Hash_ERROR on
// failure.
int sh_combine_hash(const unsigned char **hashes, size_t count,
                    unsigned char *out);

void sh_bin_to_hex(const unsigned char *hash, unsigned int len, char *out);

#endif // !HASH_H
