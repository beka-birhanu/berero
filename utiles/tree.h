#ifndef TREE_H
#define TREE_H

#include "index.h"

#define TREE_OK 0
#define TREE_ERROR -1

int dump_tree(const struct INode *i, const char *path);
const struct INode *load_tree(const char *hash);

#endif // !TREE_H
