/**
* This source file implements the "PKE Server" functionality:
 *
 *   1) Processes key registration
 *     i) Persists public key in key repository
 *   2)  Retrieves public keys
 *     i) Key is fetched from key repository
 **/

#include <stdio.h>

#include "domain/pke.h"
#include "key_repository.h"
#include "shared.h"

static DomainServer *pkeServer = NULL;

int main() {
  if (initPKEServer(&pkeServer) == ERROR) {
    printf("init failed");
    return ERROR;
  }
  if (pkeServer->base.start(&pkeServer->base) == ERROR) {
    printf("start failed");
    return ERROR;
  }

  printf("Started PKE server!\n");

  while (true) {
    ClientHandle receiveHandle;
    PKServerToLodiClient receivedMessage;

    if (pkeServer->receive(pkeServer, (UserMessage *) &receivedMessage, &receiveHandle) == ERROR) {
      printf("Failed to handle incoming PClientToPKServer message.\n");
      continue;
    }
    PKServerToPClientOrLodiServer responseMessage = {
      .userID = receivedMessage.userID,
    };

    if (receivedMessage.messageType == registerKey) {
      printf("Received registerKey message \n");
      addKey(receivedMessage.userID, receivedMessage.publicKey);
      responseMessage.messageType = ackRegisterKey;
      responseMessage.publicKey = receivedMessage.publicKey;
      printf("Req 1. b. Added publicKey=%u for userId=%u\n", responseMessage.publicKey, responseMessage.userID);
    } else if (receivedMessage.messageType == requestKey) {
      printf("Req D. 2. a. received requestKey message \n");
      unsigned int *publicKey;
      if (getKey(receivedMessage.userID, &publicKey) != SUCCESS) {
        printf("publicKey=%u not found.\n", receivedMessage.publicKey);
        responseMessage.messageType = ackPKFail;
      } else {
        responseMessage.messageType = responsePublicKey;
        responseMessage.publicKey = *publicKey;
      }
      printf("Req D. 2. b. responding to requestKey message with responsePublicKey\n");
    } else {
      printf("Warning: Received message with unknown message type. Skipping...\n");
      continue;
    }

    if (pkeServer->send(pkeServer, (UserMessage *) &responseMessage, &receiveHandle) == ERROR) {
      printf("Error while sending message.\n");
    } else {
      printf("Responded to client successfully.\n");
    }
  }
}
