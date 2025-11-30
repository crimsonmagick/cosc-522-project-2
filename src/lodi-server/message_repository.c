
#include "message_repository.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shared.h"
#include "collections/int_map.h"
#include "collections/list.h"
#include "domain/lodi.h"

IntMap *userMessages;

int addMessage(unsigned int userId, char *message) {
  if (userMessages == NULL) {
    createMap(&userMessages);
  }

  List *messages = NULL;
  const int rv = userMessages->get(userMessages, userId, (void **) &messages);
  if (rv == ERROR) {
    printf("[MessageRepository] Error while persisting user message for userId=%d; unknown map error.", userId);
    return ERROR;
  }
  if (rv == NOT_FOUND) {
    if (createList(&messages) == ERROR ||
        userMessages->add(userMessages, userId, messages) == ERROR) {
      printf("[MessageRepository] Error while persisting user message for userId=%d; failure while creating list.",
             userId);
      return ERROR;
    }
  }

  char *toPersist = malloc(LODI_MESSAGE_LENGTH* sizeof(char));
  if (toPersist == NULL) {
    printf("[MessageRepository] Error while persisting user message for userId=%d; malloc() failure.", userId);
    return ERROR;
  }
  memcpy(toPersist, message, LODI_MESSAGE_LENGTH);
  if (messages->append(messages, toPersist) == ERROR) {
    printf("[MessageRepository] Error while persisting user message for userId=%d; failed to append message.", userId);
    free(toPersist);
    return ERROR;
  }

  return SUCCESS;
}

int getMessages(const unsigned int userId, List **outMessages) {
  if (userMessages == NULL) {
    createMap(&userMessages);
  }
  return userMessages->get(userMessages, userId, (void **) outMessages);
}
