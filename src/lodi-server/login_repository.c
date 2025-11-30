#include "collections/int_map.h"
#include "login_repository.h"

#include <stdlib.h>
#include <string.h>

#include "shared.h"

IntMap *userStore = NULL;

int userLogin(const ClientHandle *userClient) {
  if (!userStore) {
    createMap(&userStore);
  }
  ClientHandle *toPersist = malloc(sizeof(ClientHandle));
  memcpy(toPersist, userClient, sizeof(ClientHandle));
  return userStore->add(userStore, userClient->userID, toPersist);
}

int userLogout(const ClientHandle *userClient) {
  if (!userStore) {
    createMap(&userStore);
  }
  return userStore->remove(userStore, userClient->userID, NULL);
}

int isUserLoggedIn(const ClientHandle *userClient) {
  if (!userStore) {
    createMap(&userStore);
  }
  DomainClient *persisted = NULL;
  if (userStore->get(userStore, userClient->userID, (void **) &persisted) == SUCCESS
      && persisted->remoteAddr.sin_addr.s_addr == userClient->clientAddr.sin_addr.s_addr) {
    return true;
  }
  return false;
}
