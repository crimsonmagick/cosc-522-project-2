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
  if (initPKEServer(&pkeServer) == ERROR
      || pkeServer->base.start(&pkeServer->base) == ERROR) {
    printf("Error, PKE server failed to start\n");
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
      printf("Req D. 1. a. received registerKey message \n");
      addKey(receivedMessage.userID, receivedMessage.publicKey);
      responseMessage.messageType = ackRegisterKey;
      responseMessage.publicKey = receivedMessage.publicKey;
      printf("Req 1. b. Added publicKey=%u for userId=%u\n", responseMessage.publicKey, responseMessage.userID);
    } else if (receivedMessage.messageType == requestKey) {
      printf("Req D. 2. a. received registerKey message \n");
      unsigned int *publicKey;
      if (getKey(receivedMessage.userID, &publicKey) == ERROR) {
        printf("publicKey=%u not found.\n", receivedMessage.publicKey);
      } else {
        responseMessage.publicKey = *publicKey;
      }
      responseMessage.messageType = responsePublicKey;
      printf("Req D. 2. b. responding to requestKey message with responsePublicKey\n");
    } else {
      printf("Warning: Received message with unknown message type.\n");
    }

    if (pkeServer->send(pkeServer, (UserMessage *) &responseMessage, &receiveHandle) == ERROR) {
      printf("Error while sending message.\n");
    } else {
      printf("Responded to client successfully.\n");
    }
  }
}
