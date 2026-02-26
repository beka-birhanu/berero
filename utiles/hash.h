#ifndef HASH_H
#define HASH_H

#include <stdio.h>

#define HASH_OK 0
#define HASH_ERROR -1

// Computes the SHA-256 of the file passed.
// Returns the hash char array.
int hash(FILE *file, unsigned char *hash);

#endif // !HASH_H
