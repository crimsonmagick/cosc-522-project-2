#ifndef COSC522_LODI_MAP_H
#define COSC522_LODI_MAP_H

/**
 * Defines the interface for a HashMap with int keys.
 */
typedef struct IntMap {
  /**
   * Gets an element.
   *
   * @param map Base map to operate on
   * @param key Element's key to lookup.
   * @param element Pointer to found element
   * @return SUCCESS, ERROR, or NOT_FOUND
   */
  int (*get)(struct IntMap *map, unsigned int key, void **element);

  /**
   * Adds an element.
   *
   * @param map Base map to operate on
   * @param key Element's key
   * @param element Pointer to allocated element
   * @return SUCCESS, ERROR
   */
  int (*add)(struct IntMap *map, unsigned int key, void *element);

  /**
  * Removes an element from the map, optionally freeing its memory.
  *
  * @param map Base map to operate on
  * @param key Element's key
  * @param element Optional pointer to allocated element. Element is freed if this pointer is null
  *
  * @return SUCCESS, ERROR
  *
  */
  int (*remove)(struct IntMap *map, unsigned int key, void **element);

  /**
   * Destroys and deallocates an IntMap.
   *
   * @param map To destroy
   */
  void (*destroy)(struct IntMap **map);
} IntMap;

/**
 * Creates a new IntMap
 * @param intMap The new HashMap
 * @return SUCCESS or ERROR
 */
int createMap(IntMap **intMap);

#endif
