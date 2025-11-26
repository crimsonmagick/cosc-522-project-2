#include "collections/list.h"
#include <stdlib.h>
#include <string.h>

#include "shared.h"

typedef struct Node {
    struct Node *prev;
    struct Node *next;
    void *element;
} Node;

typedef struct LinkedList {
    List base;
    size_t elementSize;
    Node sentinel;
} LinkedList;

static int append(List *list, void *element) {
    LinkedList *impl = (LinkedList *) list;

    Node *node = malloc(sizeof(Node));
    if (!node) {
        return ERROR;
    }

    node->element = malloc(impl->elementSize);
    if (!node->element) {
        free(node);
        return ERROR;
    }

    memcpy(node->element, element, impl->elementSize);

    // Insert before sentinel (at end)
    Node *tail = impl->sentinel.prev;
    node->next = &impl->sentinel;
    node->prev = tail;
    tail->next = node;
    impl->sentinel.prev = node;

    list->length++;
    return SUCCESS;
}

static int get(List *list, int idx, void *element) {
    LinkedList *impl = (LinkedList *) list;
    if (idx < 0 || idx >= list->length) return ERROR;

    Node *n = impl->sentinel.next;
    for (int i = 0; i < idx; i++) {
        n = n->next;
    }

    memcpy(element, n->element, impl->elementSize);
    return SUCCESS;
}

static int remove(List *list, int idx, void *element) {
    LinkedList *impl = (LinkedList *) list;
    if (idx < 0 || idx >= list->length) return ERROR;

    Node *n = impl->sentinel.next;
    for (int i = 0; i < idx; i++)
        n = n->next;

    if (element)
        memcpy(element, n->element, impl->elementSize);

    // Unlink the node
    n->prev->next = n->next;
    n->next->prev = n->prev;

    free(n->element);
    free(n);

    list->length--;
    return SUCCESS;
}

static void destroy(List **list) {
    LinkedList *impl = (LinkedList *) *list;
    Node *n = impl->sentinel.next;

    while (n != &impl->sentinel) {
        Node *next = n->next;
        free(n->element);
        free(n);
        n = next;
    }

    free(impl);
    *list = NULL;
}

int createList(List **list, size_t elementSize) {
    LinkedList *impl = malloc(sizeof(LinkedList));
    if (!impl) return ERROR;

    impl->elementSize = elementSize;
    impl->base.length = 0;
    impl->base.append = append;
    impl->base.get = get;
    impl->base.remove = remove;
    impl->base.destroy = destroy;

    // Initialize sentinel as both head and tail
    impl->sentinel.next = &impl->sentinel;
    impl->sentinel.prev = &impl->sentinel;
    impl->sentinel.element = NULL;

    *list = (List *) impl;
    return SUCCESS;
}
