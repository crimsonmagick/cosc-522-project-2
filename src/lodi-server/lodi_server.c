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

#include "follower_repository.h"
#include "message_repository.h"
#include "domain/lodi.h"
#include "shared.h"
#include "domain/pke.h"
#include "domain/tfa.h"
#include "util/rsa.h"
#include "util/server_configs.h"

static DomainClient *pkeClient = NULL;
static DomainServer *lodiServer = NULL;
static DomainClient *tfaClient = NULL;
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

  if (tfaClient->send(tfaClient, (UserMessage *) &requestMessage) == ERROR) {
    printf("Unable to send push notification, aborting...\n");
    return ERROR;
  }

  TFAServerToLodiServer response;
  if (tfaClient->receive(tfaClient, (UserMessage *) &response) == ERROR) {
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
  initPKEClientDomain(&pkeClient);
  pkeClient->base.start(&pkeClient->base);
  if (initLodiServerDomain(&lodiServer) == ERROR) {
    printf("Failed to initialize Lodi Server Domain!\n");
    exit(-1);
  }
  if (lodiServer->base.start(&lodiServer->base) == ERROR) {
    printf("Failed to start Lodi Server!\n");
    exit(-1);
  }
  initTFAClientDomain(&tfaClient, false);
  initMessageRepository();
  initFollowerRepository();
  tfaClient->base.start(&tfaClient->base);
  tfaServerAddress = getServerAddr(TFA);

  while (true) {
    PClientToLodiServer receivedMessage;
    DomainHandle remoteHandle;
    const int receivedSuccess = lodiServer->receive(lodiServer, (UserMessage *) &receivedMessage, &remoteHandle);

    if (receivedSuccess == DOMAIN_FAILURE) {
      printf("Failed to handle incoming PClientToLodiServer message.\n");
      continue;
    }
    if (receivedSuccess == TERMINATED) {
      printf("Connection terminated for userId=%d, socket %d\n", remoteHandle.userID, remoteHandle.clientSock);
      continue;
    }
    printf("Req E. 1. Received login message from Lodi client\n");

    unsigned int publicKey;
    bool authenticated = false;
    if (getPublicKey(pkeClient, receivedMessage.userID, &publicKey) == ERROR) {
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
    } else if (receivedMessage.messageType == follow) {
      addFollower(receivedMessage.recipientID, receivedMessage.userID);
      List *followers;
      int follwGetRt = getIdolFollowers(receivedMessage.recipientID, &followers);
      printf("Followers for idolId=%u\n", receivedMessage.recipientID);
      for (int i = 0; i < followers->length && follwGetRt == SUCCESS; i++) {
        int *userId;
        followers->get(followers, i, (void **) &userId);
        printf("followerId=%u\n", *userId);
      }
      List *idols;
      int idolGetRt = getFollowerIdols(receivedMessage.userID, &idols);
      printf("Idols for followerId=%u\n", receivedMessage.userID);
      for (int i = 0; i < idols->length && idolGetRt == SUCCESS; i++) {
        int *idolId;
        idols->get(idols, i, (void **) &idolId);
        printf("idolId=%u\n", *idolId);
      }
    } else if (receivedMessage.messageType == unfollow) {
      removeFollower(receivedMessage.recipientID, receivedMessage.userID);
      List *followers;
      int getRv = getIdolFollowers(receivedMessage.recipientID, &followers);
      printf("Followers for idolId=%u\n", receivedMessage.recipientID);
      for (int i = 0; i < followers->length && getRv == SUCCESS; i++) {
        int *userId;
        followers->get(followers, i, (void **) &userId);
        printf("userId=%u\n", *userId);
      }
    }

    LodiServerMessage responseMessage = {
      .messageType = responseMessageType,
      .userID = receivedMessage.userID,
      .message = "Hello from Lodi Server!"
    };

    const int sendSuccess = lodiServer->send(lodiServer, (UserMessage *) &responseMessage, &remoteHandle);
    if (sendSuccess == ERROR) {
      printf("Error while sending Lodi login response.\n");
    }
  }
}
