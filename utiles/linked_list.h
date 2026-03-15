#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stddef.h>

#define LL_OK 1
#define LL_ERROR 0

#define LL_DIR_BACK 0
#define LL_DIR_FRONT 1

struct LinkedList;
struct Node;

typedef void (*value_destructor)(void *value);

/* Allocate a new empty list. Returns NULL on failure. */
struct LinkedList *ll_new();

/* Return the next node in the list, or NULL if none. */
const struct Node *ll_iter(struct LinkedList *ll);

/* Reset the iterator to the beginning of the list.*/
void ll_reset_iter(struct LinkedList *ll);

/* Free the list and all of its nodes. No-op if ll is NULL. */
void ll_free(struct LinkedList *ll);

/* Append a key/value pair to the back of the list. Returns LL_OK on success,
 * LL_ERROR on failure. value is stored as-is; caller looses ownership. */
int ll_append(struct LinkedList *ll, const char *key, void *value,
              value_destructor destructor);

/* Insert a key/value pair at the front of the list. Returns LL_OK on success,
 * LL_ERROR on failure. value is stored as-is; caller looses ownership. */
int ll_push_front(struct LinkedList *ll, const char *key, void *value,
                  value_destructor destructor);

/* Remove the first node whose key matches. Returns LL_OK if removed, LL_ERROR
 * if not found. */
int ll_remove(struct LinkedList *ll, const char *key);

/* Remove front (dir=LL_DIR_FRONT) or back (dir=LL_DIR_BACK) node. Returns Node
 * on success, NULL on failure. Caller must ll_node_free() the returned pointer.
 */
struct Node *ll_pop(struct LinkedList *ll, int dir);

/* Return the first value with the given key, or NULL if not found. */
const struct Node *ll_find(const struct LinkedList *ll, const char *key);

/* Swap the key and value of two nodes in place. No-op if either node is NULL or
 * n1 == n2. */
void ll_swap_pos(struct Node *n1, struct Node *n2);

/* Return the number of elements in the list, or 0 if ll is NULL. */
size_t ll_len(const struct LinkedList *ll);

/* Return the key of a node, or NULL if node is NULL. */
const char *ll_node_key(const struct Node *node);

/* Return the value of a node, or NULL if node is NULL. */
void *ll_node_value(const struct Node *node);

/* Free the node. No-op if node is NULL. */
void ll_node_free(struct Node *node);

/* Return a newly allocated string representation (one line per node,
 * "  key => value\n"), or NULL if ll is NULL or allocation fails. Caller
 * must free() the result. Empty list returns allocated "". */
char *ll_stringify(const struct LinkedList *ll);

/* Print each node as "  key => value" to stdout. No-op if ll is NULL. */
void ll_print(const struct LinkedList *ll);

#endif /* LINKED_LIST_H */
