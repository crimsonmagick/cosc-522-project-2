#ifndef COSC522_LODI_MAP_H
#define COSC522_LODI_MAP_H

/**
 * Defines the interface for a HashMap with int keys.
 */
typedef struct IntMap {

  int (*get)(struct IntMap * map, unsigned int key, void **element);
  int (*add)(struct IntMap * map, unsigned int key, void *element);
  int (*remove)(struct IntMap *map, unsigned int key, void **element);
  void (*destroy)(struct IntMap **map);
} IntMap;

/**
 * Creates a new IntMap
 * @param intMap The new HashMap
 * @return SUCCESS or ERROR
 */
int createMap(IntMap **intMap);

#endif
