#ifndef COSC522_LODI_MAP_H
#define COSC522_LODI_MAP_H

typedef struct IntMap {
  int (*get)(struct IntMap * map, unsigned int key, void **element);
  int (*add)(struct IntMap * map, unsigned int key, void *element);
  int (*remove)(struct IntMap *map, unsigned int key, void **element);
  void (*destroy)(struct IntMap **map);
} IntMap;

int createMap(IntMap **intMap);

#endif
