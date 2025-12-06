/**
 * See `login_repository.h`
 */

#include "collections/int_map.h"
#include "login_repository.h"

#include <stdio.h>
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
  ClientHandle *persisted = NULL;
  int rv = userStore->remove(userStore, userClient->userID, (void **) &persisted);
  if (rv == SUCCESS) {
    free(persisted);
  } else {
    printf("Error: unable to remove logged-in client handle, rv=%d\n", rv);
    return ERROR;
  }
  return SUCCESS;
}

int isUserLoggedIn(const ClientHandle *userClient) {
  if (!userStore) {
    createMap(&userStore);
  }
  ClientHandle *persisted = NULL;
  if (userStore->get(userStore, userClient->userID, (void **) &persisted) == SUCCESS
      && persisted->clientAddr.sin_addr.s_addr == userClient->clientAddr.sin_addr.s_addr) {
    return true;
  }
  return false;
}
