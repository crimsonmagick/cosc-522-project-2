#ifndef COSC522_LODI_LINKED_LIST_H
#define COSC522_LODI_LINKED_LIST_H
#include <stddef.h>

/**
 * Defines the interface for the Linked List
 */
typedef struct List {
  int length; // length of the list - can't be less than 0

  /**
   * Appends a single element to the list
   *
   * @param list Base list to append to
   * @param element Element to append
   * @return SUCCESS or ERROR
   */
  int (*append)(struct List *list, void *element);

  /**
   * Gets a single element from the list
   *
   * @param list Base list
   * @param element Pointer to stored element
   * @return SUCCESS or ERROR
   */
  int (*get)(struct List *list, int idx, void **element);

  /**
   * Removes an item from a list, and optionally frees its memory. Optionally retrieves the element for the caller.
   *
   * @param list Base list
   * @param element Optional pointer to stored element. If non-null, the caller must manage the lifecycle of the element.
   * @return SUCCESS or ERROR
   */
  int (*remove)(struct List *list, int idx, void **element);

  /**
  * Deallocates the list.
  *
  * @param list Base list
  * @return SUCCESS or ERROR
  */
  void (*destroy)(struct List **list);
} List;

/**
 * Creates a new Linked List.
 *
 * @param list The new Linked List
 * @return SUCCESS or ERROR
 */
int createList(List **list);

#endif
