/**
* Provides persistence for registered client ip addresses and ports
 **/

#include <string.h>

#include "message_repository.h"

#include <stdio.h>
#include <stdlib.h>

#include "shared.h"

#define SIZE 500
#define MAX_MESSAGE_COUNT 100
#define MESSAGE_SIZE 100

char **messages[SIZE];
int messageCounts[SIZE];

/**
 *  Constructor
 */
void initRepository() {
  memset(messages, 0, SIZE * sizeof(char **));
  memset(messageCounts, 0, SIZE * sizeof(int));
}


int addMessage(unsigned int userId, char *message) {
  const unsigned int idx = userId % SIZE;
  if (messages[idx] == NULL) {
    messages[idx] = malloc(sizeof(char *) * MAX_MESSAGE_COUNT);
    for (int i = 0; i < MAX_MESSAGE_COUNT; i++) {
      messages[idx][i] = malloc(MESSAGE_SIZE);
    }
  }
  if (messageCounts[userId] < MAX_MESSAGE_COUNT) {
    memcpy(messages[idx][messageCounts[userId]], message, MESSAGE_SIZE);
    messageCounts[userId]++;
  } else {
    printf("Warning: max messages reached for user with userId=%u. Skipping persistence.", userId);
  }
  return SUCCESS;
}

int getMessages(unsigned int userId, char ***outMessages, int *messageCount) {
  const unsigned int idx = userId % SIZE;
  if (messages[idx] == NULL) {
    // key not found
    return ERROR;
  }
  *outMessages = messages[idx];
  *messageCount = messageCounts[userId];

  return SUCCESS;
}
