#ifndef HASH_H
#define HASH_H

#include <stdio.h>

#define HASH_OK 0
#define HASH_ERROR -1

// Computes the SHA-256 of the file passed.
// Returns Hash_OK on success, Hash_ERROR on failure.
int sh_hash(FILE *file, unsigned char *hash);

#endif // !HASH_H
