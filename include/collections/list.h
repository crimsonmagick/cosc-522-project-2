#ifndef COSC522_LODI_LINKED_LIST_H
#define COSC522_LODI_LINKED_LIST_H
#include <stddef.h>

/**
 * Defines the interface for the Linked List
 */
typedef struct List {
  int length;
  int (*append)(struct List *list, void *element);
  int (*get)(struct List *list, int idx, void **element);
  int (*remove)(struct List *list, int idx, void **element);
  void (*destroy)(struct List **list);
} List;

/**
 * Creates a new Linked List
 * @param list The new Linked List
 * @return SUCCESS or ERROR
 */
int createList(List **list);



#endif
