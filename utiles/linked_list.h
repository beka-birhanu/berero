#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stddef.h>

#define LL_OK 1
#define LL_ERROR 0

#define LL_DIR_BACK 0
#define LL_DIR_FRONT 1

struct LinkedList;
struct Node;

/* Allocate a new empty list. Returns NULL on failure. */
struct LinkedList *ll_new(void);

/* Return the next node in the list, or NULL if none. The returned pointer is
 * valid until ll_next is called again, or ll_free is called. */
struct Node *ll_iter(struct LinkedList *ll);

/* Reset the iterator to the beginning of the list.*/
void ll_reset_iter(struct LinkedList *ll);

/* Free the list and all of its nodes. No-op if ll is NULL. */
void ll_free(struct LinkedList *ll);

/* Append a key/value pair to the back of the list. Returns LL_OK on success,
 * LL_ERROR on failure. value is stored as-is; caller retains ownership. */
int ll_append(struct LinkedList *ll, const char *key, void *value);

/* Insert a key/value pair at the front of the list. Returns LL_OK on success,
 * LL_ERROR on failure. value is stored as-is; caller retains ownership. */
int ll_push_front(struct LinkedList *ll, const char *key, void *value);

/* Remove the first node whose key matches. Returns LL_OK if removed, LL_ERROR
 * if not found. */
int ll_remove(struct LinkedList *ll, const char *key);

/* Remove front (dir=LL_DIR_FRONT) or back (dir=LL_DIR_BACK) node. Returns LL_OK
 * on success, LL_ERROR on failure. */
int ll_pop(struct LinkedList *ll, int dir);

/* Return the first node with the given key, or NULL if not found. */
struct Node *ll_find(const struct LinkedList *ll, const char *key);

/* Swap the key and value of two nodes in place. No-op if either node is NULL or
 * n1 == n2. */
void ll_swap_pos(struct Node *n1, struct Node *n2);

/* Return the number of elements in the list, or 0 if ll is NULL. */
size_t ll_len(const struct LinkedList *ll);

/* Return the key of a node, or NULL if node is NULL. */
const char *ll_node_key(const struct Node *node);

/* Return the value of a node, or NULL if node is NULL. */
void *ll_node_value(const struct Node *node);

/* Return a newly allocated string representation (one line per node,
 * "  key => value\n"), or NULL if ll is NULL or allocation fails. Caller
 * must free() the result. Empty list returns allocated "". */
char *ll_stringify(const struct LinkedList *ll);

/* Print each node as "  key => value" to stdout. No-op if ll is NULL. */
void ll_print(const struct LinkedList *ll);

#endif /* LINKED_LIST_H */
