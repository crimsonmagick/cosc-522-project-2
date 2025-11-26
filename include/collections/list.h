#ifndef COSC522_LODI_LINKED_LIST_H
#define COSC522_LODI_LINKED_LIST_H
#include <stddef.h>

typedef struct List {
  int length;
  int (*append)(struct List * list, void *element);
  int (*get)(struct List * list, int idx, void *element);
  int (*remove)(struct List *, int idx, void *element);
  void (*destroy)(struct List **list);
} List;

int createList(List **list, size_t elementSize);

#endif
