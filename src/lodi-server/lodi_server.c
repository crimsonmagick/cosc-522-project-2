/**
* This source file implements the "Lodi Server" functionality:
 *
 * Handles Lodi login:
 *   1)  Authenticates the client's digital signature against the associated publicKey
 *   2)  Performs TFA after the digital signature is validated
 *   3)  Responds to the user with a "success" message if both phases succeed
 **/

#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdbool.h>

#include "message_repository.h"
#include "messaging/lodi_messaging.h"
#include "shared.h"
#include "messaging/pke_messaging.h"
#include "messaging/tfa_messaging.h"
#include "util/rsa.h"
#include "util/server_configs.h"

static DomainService *pkeDomain = NULL;
static struct sockaddr_in pkServerAddress;
static StreamDomainServiceHandle *lodiDomain = NULL;
static struct sockaddr_in lodiServerAddress;
static DomainService *tfaDomain = NULL;
static struct sockaddr_in tfaServerAddress;

/**
 * Send a push request to the TFA server
 * @param userID user to authenticate
 * @return ERROR or SUCCESS
 */
int sendPushRequest(const unsigned int userID) {
  const TFAClientOrLodiServerToTFAServer requestMessage = {
    .messageType = requestAuth,
    .userID = userID
  };

  if (toDatagramDomainHost(tfaDomain, (void *) &requestMessage, &tfaServerAddress) == ERROR) {
    printf("Unable to send push notification, aborting...\n");
    return ERROR;
  }

  TFAServerToLodiServer response;
  struct sockaddr_in receiveAddr;
  if (fromDatagramDomainHost(tfaDomain, &response, &receiveAddr) == ERROR) {
    printf("Unable to receive push notification response, aborting...\n");
    return ERROR;
  }

  printf("Push auth confirmation received! Received: messageType=%u, userID=%u\n", response.messageType,
         response.userID);

  return SUCCESS;
}

/**
 * Lodi Server infinite loop
 */
int main() {
  initPKEClientDomain(&pkeDomain);
  if (initLodiServerDomain(&lodiDomain) == ERROR) {
    printf("Failed to initialize Lodi Server Domain!\n");
    exit(-1);
  }
  initTFAClientDomain(&tfaDomain, false);
  pkServerAddress = getServerAddr(PK);
  lodiServerAddress = getServerAddr(LODI);
  tfaServerAddress = getServerAddr(TFA);

  while (true) {
    PClientToLodiServer receivedMessage;
    const int receivedSuccess = fromStreamDomainHost(lodiDomain, &receivedMessage);

    if (receivedSuccess == DOMAIN_FAILURE) {
      printf("Failed to handle incoming PClientToLodiServer message.\n");
      stopStreamService(&lodiDomain);
      exit(-1);
    }
    printf("Req E. 1. Received login message from Lodi client\n");

    unsigned int publicKey;
    bool authenticated = false;
    if (getPublicKey(pkeDomain, &pkServerAddress, receivedMessage.userID, &publicKey) == ERROR) {
      printf("Failed to retrieve public key!\n");
    } else {
      printf("E 1. a. 1) a. and b. sent requestPublicKey, received responsePublicKey");
      const unsigned long decrypted = decryptTimestamp(receivedMessage.digitalSig, publicKey, MODULUS);
      if (decrypted == receivedMessage.timestamp) {
        authenticated = true;
        printf("Req E 1. a. 1) b) Decrypted timestamp successfully! timestamp=%lu \n", decrypted);
      } else {
        printf("Failed to decrypt timestamp! timestamp=%lu, decrypted=%lu \n",
               receivedMessage.timestamp, decrypted);
      }
    }

    /*
     * IMPORTANT NOTE!!! E 1. c. 1) mentions sending a corresponding ERROR message.
     * This *cannot* be done as the struct enum LodiServerToLodiClientAcks does not have a failure status!!!
     * Opted to allow clients to use timeouts as failures instead. It was a choice between the requirement of using
     * the structs + enums or expanding it. We chose to keep the struct as-is.
     */
    enum LodiServerMessageType responseMessageType = failure;
    if (authenticated && receivedMessage.messageType == login) {
      printf("Verifying login with TFA...\n");
      if (sendPushRequest(receivedMessage.userID) == ERROR) {
        printf("Req E 1. c. 1 Failed to authenticate with push confirmation! Continuing without response...\n");
        continue;
      }
      responseMessageType = ackLogin;
      printf("Req E 1. b. 1) Validated TFA successfully!\n");
    } else if (receivedMessage.messageType == login) {
      printf("Req E 1. c. 1) b)Failed to authenticate with public key! Continuing without response...\n");
      continue;
    } else if (receivedMessage.messageType == post) {
      printf("Persisting idol message, message=%s...\n", receivedMessage.message);
      addMessage(receivedMessage.userID, receivedMessage.message);
      char **messages;
      int messageCount;
      getMessages(receivedMessage.userID, &messages, &messageCount);
      for (int i = 0; i < messageCount; i++) {
        printf("Message %d: %s\n", i, messages[i]);
      }
      responseMessageType = ackPost;
    }

    LodiServerMessage responseMessage = {
      .messageType = responseMessageType,
      .userID = receivedMessage.userID,
      .message = "Hello from Lodi Server!"
    };

    const int sendSuccess = toStreamDomainHost(lodiDomain, &responseMessage);
    if (sendSuccess == ERROR) {
      printf("Error while sending Lodi login response.\n");
    }
  }
}
