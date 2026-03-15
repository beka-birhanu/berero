#include "linked_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node *_ll_find(const struct LinkedList *ll, const char *key);

struct Node {
  char *key;
  void *value;
  struct Node *prev;
  struct Node *next;
  value_destructor destructor;
};

struct LinkedList {
  size_t len;
  struct Node *head;
  struct Node *tail;
  struct Node *_current;
};

struct LinkedList *ll_new(void) {
  struct LinkedList *list = malloc(sizeof(*list));
  if (!list)
    return NULL;

  struct Node *head = malloc(sizeof(*head));
  struct Node *tail = malloc(sizeof(*tail));
  if (!head || !tail) {
    free(head);
    free(tail);
    free(list);
    return NULL;
  }

  head->key = NULL;
  head->value = NULL;
  head->prev = NULL;
  head->next = tail;

  tail->key = NULL;
  tail->value = NULL;
  tail->prev = head;
  tail->next = NULL;

  list->head = head;
  list->tail = tail;
  list->len = 0;
  list->_current = head;

  return list;
}

const struct Node *ll_iter(struct LinkedList *ll) {
  if (!ll || !ll->_current)
    return NULL;

  ll->_current = ll->_current->next;
  if (ll->_current == ll->tail)
    ll->_current = NULL;

  return ll->_current;
}

void ll_reset_iter(struct LinkedList *ll) {
  if (!ll)
    return;

  ll->_current = ll->head;
}

void ll_free(struct LinkedList *ll) {
  if (!ll)
    return;

  struct Node *cur = ll->head;
  while (cur) {
    struct Node *next = cur->next;
    free(cur->key);
    if (cur->destructor)
      cur->destructor(cur->value);
    free(cur);
    cur = next;
  }
  free(ll);
}

int ll_append(struct LinkedList *ll, const char *key, void *value,
              value_destructor destructor) {
  if (!ll || !key || !value)
    return LL_ERROR;

  struct Node *node = malloc(sizeof(*node));
  if (!node)
    return LL_ERROR;

  node->key = strdup(key);
  if (!node->key) {
    free(node);
    return LL_ERROR;
  }
  node->value = value;
  node->destructor = destructor;

  struct Node *last = ll->tail->prev;
  last->next = node;
  node->prev = last;
  node->next = ll->tail;
  ll->tail->prev = node;

  ++ll->len;
  return LL_OK;
}

int ll_push_front(struct LinkedList *ll, const char *key, void *value,
                  value_destructor destructor) {
  if (!ll || !key || !value)
    return LL_ERROR;

  struct Node *node = malloc(sizeof(*node));
  if (!node)
    return LL_ERROR;

  node->key = strdup(key);
  if (!node->key) {
    free(node);
    return LL_ERROR;
  }
  node->value = value;
  node->destructor = destructor;

  struct Node *first = ll->head->next;
  node->next = first;
  first->prev = node;
  ll->head->next = node;
  node->prev = ll->head;

  ++ll->len;
  return LL_OK;
}

inline struct Node *_ll_find(const struct LinkedList *ll, const char *key) {
  if (!ll || !key)
    return NULL;

  for (struct Node *cur = ll->head->next; cur != ll->tail; cur = cur->next) {
    if (cur->key && strcmp(cur->key, key) == 0)
      return cur;
  }
  return NULL;
}

const struct Node *ll_find(const struct LinkedList *ll, const char *key) {
  return _ll_find(ll, key);
}

int ll_remove(struct LinkedList *ll, const char *key) {
  struct Node *node = _ll_find(ll, key);
  if (!node)
    return LL_ERROR;

  node->prev->next = node->next;
  node->next->prev = node->prev;

  free(node->key);
  if (node->destructor)
    node->destructor(node->value);
  free(node);
  --ll->len;

  return LL_OK;
}

struct Node *ll_pop(struct LinkedList *ll, int dir) {
  if (!ll || ll->len == 0 || (dir != LL_DIR_FRONT && dir != LL_DIR_BACK))
    return LL_ERROR;

  struct Node *node = (dir == LL_DIR_FRONT) ? ll->head->next : ll->tail->prev;

  node->prev->next = node->next;
  node->next->prev = node->prev;

  --ll->len;

  return node->value;
}

void ll_swap_pos(struct Node *n1, struct Node *n2) {
  if (!n1 || !n2 || n1 == n2)
    return;

  char *tmp_key = n1->key;
  void *tmp_value = n1->value;

  n1->key = n2->key;
  n1->value = n2->value;

  n2->key = tmp_key;
  n2->value = tmp_value;
}

size_t ll_len(const struct LinkedList *ll) { return ll ? ll->len : 0; }

const char *ll_node_key(const struct Node *node) {
  return node ? node->key : NULL;
}

void *ll_node_value(const struct Node *node) {
  return node ? node->value : NULL;
}

void ll_node_free(struct Node *node) {
  if (!node)
    return;

  free(node->key);
  free(node->value);
  free(node);
}

char *ll_stringify(const struct LinkedList *ll) {
  if (!ll)
    return NULL;

  /* Each value printed as %p needs at most ~24 chars (0x + 16 hex + null) */
  size_t len = 0;
  for (struct Node *cur = ll->head->next; cur != ll->tail; cur = cur->next) {
    if (cur->key)
      len += strlen(cur->key) + 4 + 24 + 1;
  }

  char *out = malloc(len + 1);
  if (!out)
    return NULL;
  if (len == 0) {
    *out = '\0';
    return out;
  }

  char *p = out;
  for (struct Node *cur = ll->head->next; cur != ll->tail; cur = cur->next) {
    if (cur->key) {
      p += sprintf(p, "%s => %p ", cur->key, (void *)cur->value);
    }
  }
  *p = '\0';
  return out;
}

void ll_print(const struct LinkedList *ll) {
  char *s = ll_stringify(ll);
  if (!s)
    return;

  fputs(s, stdout);
  fputc('\n', stdout);
  free(s);
}
