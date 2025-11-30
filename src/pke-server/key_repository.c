/**
 * Provides persistence for registered User public keys
 **/

#include <string.h>

#include "key_repository.h"
#include "shared.h"

#define SIZE 500

unsigned int keyStore[SIZE];

/**
 * Constructor function
 */
void initKeyRepository() {
  memset(keyStore, 0, SIZE * sizeof(unsigned int));
}

/**
 * Persists a public key
 * @param userId
 * @param publicKey
 * @return ERROR, SUCCESS
 */
int addKey(unsigned int userId, unsigned int publicKey) {
  const unsigned int idx = userId % SIZE;
  keyStore[idx] = publicKey;
  return SUCCESS;
}

/**
 * Retrieves publicKey
 * @param userId user to retrieve for
 * @param publicKey  output, the public key
 * @return ERROR, SUCCESS
 */
int getKey(unsigned int userId, unsigned int *publicKey) {
  const unsigned int idx = userId % SIZE;
  const unsigned int retrieved = keyStore[idx];
  if (retrieved == 0) {
    // key not found
    return ERROR;
  }
  *publicKey = retrieved;
  return SUCCESS;
}
