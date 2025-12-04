/**
* See map.h
 */

#include <stdlib.h>

#include "collections/int_map.h"
#include "collections/list.h"
#include "shared.h"

#define BUCKETS 100

typedef struct KeyValue {
    unsigned int key;
    void *value; // pointer to caller-owned data
} KeyValue;

typedef struct IntMapImpl {
    IntMap base;
    List *buckets[BUCKETS];
} IntMapImpl;

static unsigned int hash(const unsigned int key) {
    return key % BUCKETS;
}

static int map_get(IntMap *map, const unsigned int key, void **element) {
    if (!element) return ERROR;

    const IntMapImpl *impl = (IntMapImpl *) map;
    List *list = impl->buckets[hash(key)];
    if (!list) {
        return NOT_FOUND;
    }

    KeyValue *kv;
    for (int i = 0; i < list->length; i++) {
        if (list->get(list, i, (void **) &kv) == SUCCESS && kv->key == key) {
            *element = kv->value;
            return SUCCESS;
        }
    }
    return NOT_FOUND;
}

static int map_add(IntMap *map, const unsigned int key, void *element) {
    if (!element) {
        return ERROR;
    }

    IntMapImpl *impl = (IntMapImpl *) map;
    const unsigned int h = hash(key);

    // Lazy bucket creation
    if (!impl->buckets[h]) {
        if (createList(&impl->buckets[h]) != SUCCESS)
            return ERROR;
    }

    KeyValue *kv = malloc(sizeof(KeyValue));
    if (!kv) {
        return ERROR;
    }

    kv->key = key;
    kv->value = element;

    return impl->buckets[h]->append(impl->buckets[h], kv);
}

static int map_remove(IntMap *map, const unsigned int key, void **out) {
    IntMapImpl *impl = (IntMapImpl *) map;
    List *list = impl->buckets[hash(key)];
    if (!list) return ERROR;

    KeyValue *kv;
    for (int i = 0; i < list->length; i++) {
        if (list->get(list, i, (void **) &kv) == SUCCESS && kv->key == key) {
            if (out)
                *out = kv->value; // caller owns
            else
                free(kv->value); // map owns

            free(kv); // free wrapper

            return list->remove(list, i, NULL);
        }
    }
    return NOT_FOUND;
}

static void map_destroy(IntMap **map) {
    if (!map || !*map) return;

    IntMapImpl *impl = (IntMapImpl *) (*map);

    for (int i = 0; i < BUCKETS; i++) {
        if (impl->buckets[i]) {
            // free KeyValue structs (but not values)
            KeyValue *kv;
            List *lst = impl->buckets[i];
            while (lst->remove(lst, 0, (void **) &kv) == SUCCESS) {
                free(kv); // free wrapper only
            }
            lst->destroy(&lst);
        }
    }

    free(impl);
    *map = NULL;
}

int createMap(IntMap **map) {
    if (!map) return ERROR;

    IntMapImpl *impl = malloc(sizeof(IntMapImpl));
    if (!impl) return ERROR;

    // Initialize buckets
    for (int i = 0; i < BUCKETS; i++)
        impl->buckets[i] = NULL;

    impl->base.get = map_get;
    impl->base.add = map_add;
    impl->base.remove = map_remove;
    impl->base.destroy = map_destroy;

    *map = (IntMap *) impl;
    return SUCCESS;
}
