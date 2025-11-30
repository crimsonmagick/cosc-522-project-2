/**
 * Provides persistence for registered User public keys
 **/
#include <stdlib.h>

#include "collections/int_map.h"
#include "key_repository.h"
#include "shared.h"

IntMap *keyStore = NULL;

/**
 * Persists a public key
 * @param userId
 * @param publicKey
 * @return ERROR, SUCCESS
 */
int addKey(unsigned int userId, unsigned int publicKey) {
  if (!keyStore) {
    createMap(&keyStore);
  }
  unsigned *toPersist = malloc(sizeof(unsigned int));
  *toPersist = publicKey;
  keyStore->add(keyStore, userId, toPersist);
  return SUCCESS;
}

/**
 * Retrieves publicKey
 * @param userId user to retrieve for
 * @param publicKey  output, the public key
 * @return ERROR, SUCCESS
 */
int getKey(unsigned int userId, unsigned int **publicKey) {
  if (!keyStore) {
    createMap(&keyStore);
  }
  return keyStore->get(keyStore, userId, (void **) publicKey);
}
