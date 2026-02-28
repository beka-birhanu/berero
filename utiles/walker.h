#ifndef WALKER_H
#define WALKER_H

#include "linked_list.h"
#include <stddef.h>

#define WALK_OK 1
#define WALK_ERROR 0

struct LinkedList *walk(const char *path);

int is_path_in_cwd(const char *path);

#endif // WALKER_H
