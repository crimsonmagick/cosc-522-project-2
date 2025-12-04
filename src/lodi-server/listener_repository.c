/*
 * See listener_repository.h
 */

#include <stdio.h>
#include <stdlib.h>

#include "listener_repository.h"

#include <string.h>

#include "shared.h"

static List *listeners = NULL;

/**
 *  Constructor
 */
void initListenerRepository() {
  createList(&listeners);
}

int addListener(ClientHandle *listener) {
  ClientHandle *toAppend = malloc(sizeof(ClientHandle));
  memcpy(toAppend, listener, sizeof(ClientHandle));
  listeners->append(listeners, toAppend);
  return SUCCESS;
}

int removeListener(ClientHandle *listener) {
  for (int i = 0; i < listeners->length; i++) {
    ClientHandle *removalCandidate = NULL;
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
  *listenersOut = listeners;
  return SUCCESS;
}
