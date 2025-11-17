/**
* This source file implements the "PKE Server" functionality:
 *
 *   1) Processes key registration
 *     i) Persists public key in key repository
 *   2)  Retrieves public keys
 *     i) Key is fetched from key repository
 **/

#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdbool.h>

#include "messaging/pke_messaging.h"
#include "key_repository.h"
#include "shared.h"

static DatagramDomainServiceHandle *pkeDomain = NULL;

int main() {
  initPKEServerDomain(&pkeDomain);
  initRepository();

  while (true) {
    struct sockaddr_in clientAddress;
    PKServerToLodiClient receivedMessage;
    const int receivedSuccess = fromDatagramDomainHost(pkeDomain, &receivedMessage, &clientAddress);

    if (receivedSuccess == ERROR) {
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
      unsigned int publicKey;
      if (getKey(receivedMessage.userID, &publicKey) == ERROR) {
        printf("publicKey=%u not found.\n", receivedMessage.publicKey);
      } else {
        responseMessage.publicKey = publicKey;
      }
      responseMessage.messageType = responsePublicKey;
      printf("Req D. 2. b. responding to requestKey message with responsePublicKey\n");
    } else {
      printf("Warning: Received message with unknown message type.\n");
    }

    const int sendSuccess = toDatagramDomainHost(pkeDomain, &responseMessage, &clientAddress);
    if (sendSuccess == ERROR) {
      printf("Error while sending message.\n");
    } else {
      printf("Responded to client successfully.\n");
    }
  }
}
