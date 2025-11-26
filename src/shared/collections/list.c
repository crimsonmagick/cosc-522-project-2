#include "collections/list.h"
#include <stdlib.h>

#include "shared.h"

typedef struct Node {
    struct Node *prev;
    struct Node *next;
    void *element; // Caller-owned pointer
} Node;

typedef struct LinkedList {
    List base;
    Node sentinel;
} LinkedList;

static int append(List *list, void *element) {
    if (!element) return ERROR;

    LinkedList *impl = (LinkedList *) list;

    Node *node = malloc(sizeof(Node));
    if (!node) return ERROR;

    node->element = element;

    Node *tail = impl->sentinel.prev;
    node->next = &impl->sentinel;
    node->prev = tail;
    tail->next = node;
    impl->sentinel.prev = node;

    list->length++;
    return SUCCESS;
}

static int get(List *list, int idx, void **element) {
    LinkedList *impl = (LinkedList *) list;
    if (idx < 0 || idx >= list->length || !element) return ERROR;

    Node *n = impl->sentinel.next;
    for (int i = 0; i < idx; i++)
        n = n->next;

    *element = n->element;
    return SUCCESS;
}

static int remove(List *list, int idx, void **element) {
    LinkedList *impl = (LinkedList *) list;
    if (idx < 0 || idx >= list->length) return ERROR;

    Node *n = impl->sentinel.next;
    for (int i = 0; i < idx; i++)
        n = n->next;

    if (element)
        *element = n->element;   // Caller reclaims ownership
    // else: caller doesn't want it; we do NOT free it

    n->prev->next = n->next;
    n->next->prev = n->prev;

    free(n);
    list->length--;
    return SUCCESS;
}

static void destroy(List **list) {
    if (!list || !*list) return;

    LinkedList *impl = (LinkedList *) *list;
    Node *n = impl->sentinel.next;

    while (n != &impl->sentinel) {
        Node *next = n->next;
        // We do NOT free n->element
        free(n);
        n = next;
    }

    free(impl);
    *list = NULL;
}

int createList(List **list) {
    if (!list) return ERROR;

    LinkedList *impl = malloc(sizeof(LinkedList));
    if (!impl) return ERROR;

    impl->base.length = 0;
    impl->base.append = append;
    impl->base.get = get;
    impl->base.remove = remove;
    impl->base.destroy = destroy;

    impl->sentinel.next = &impl->sentinel;
    impl->sentinel.prev = &impl->sentinel;
    impl->sentinel.element = NULL;

    *list = (List *) impl;
    return SUCCESS;
}
