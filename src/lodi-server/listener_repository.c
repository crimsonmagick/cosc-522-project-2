#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include "collections/list.h"
#include "collections/int_map.h"
#include "listener_repository.h"
#include "shared.h"

static List *listeners = NULL;

/**
 *  Constructor
 */
void initListenerRepository() {
  createList(&listeners);
}

int addListener(DomainHandle * listener) {
  DomainHandle * toAppend = malloc(sizeof(DomainHandle));
  toAppend->userID = listener->userID;
  toAppend->clientSock = listener->clientSock;
  listeners->append(listeners, toAppend);
  return SUCCESS;
}

int removeListener(DomainHandle *listener) {
  for (int i = 0; i < listeners->length; i++) {
    DomainHandle *removalCandidate = NULL;
    listeners->get(listeners, i, (void **) &removalCandidate);
    if (removalCandidate == NULL) {
      printf("Unexpected error while retrieving listeners...\n");
      return ERROR;
    }
    if (removalCandidate->userID == listener->userID
      && removalCandidate->clientSock == listener->clientSock) {
      listeners->remove(listeners, i, NULL);
      free(removalCandidate);
      return SUCCESS;
    }
  }
  return SUCCESS;
}

int getAllListeners(List **listenersOut) {
  *listenersOut= listeners;
  return SUCCESS;
}
