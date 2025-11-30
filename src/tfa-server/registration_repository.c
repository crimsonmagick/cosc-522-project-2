/**
* Provides persistence for registered client ip addresses and ports
 **/

#include "registration_repository.h"
#include <stdlib.h>
#include <string.h>

#include "collections/int_map.h"


static IntMap *clientStore = NULL;


/**
 * Registers a client callback
 *
 * @param userId
 * @param clientHandleIn
 * @return
 */
int registerClient(const unsigned int userId, const ClientHandle *clientHandleIn) {
  if (!clientStore) {
    createMap(&clientStore);
  }
  ClientHandle *toPersist = malloc(sizeof(ClientHandle));
  memcpy(toPersist, clientHandleIn, sizeof(ClientHandle));

  return clientStore->add(clientStore, userId, (void *) toPersist);
}

/**
 * Gets the IP and port
 *
 * @param userId
 * @param clientHandleOut
 * @return
 */
int getClient(const unsigned int userId, ClientHandle **clientHandleOut) {
  if (!clientStore) {
    createMap(&clientStore);
  }
  return clientStore->get(clientStore, userId, (void **) clientHandleOut);
}
