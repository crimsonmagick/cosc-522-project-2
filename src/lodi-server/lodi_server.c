/**
* This source file implements the "Lodi Server" functionality:
 *
 * Handles Lodi login:
 *   1)  Authenticates the client's digital signature against the associated publicKey
 *   2)  Performs TFA after the digital signature is validated
 *   3)  Responds to the user with a "success" message if both phases succeed
 * After login, handles Lodi Post, Follow, Unfollow, and Logout
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "domain/lodi.h"
#include "domain/pke.h"
#include "domain/tfa.h"
#include "shared.h"
#include "util/rsa.h"

#include "follower_repository.h"
#include "listener_repository.h"
#include "login_repository.h"
#include "message_repository.h"

static int authenticate(PClientToLodiServer *request);

static void pushFeedMessage(unsigned int idolId, char *message);

static void handleFeed(unsigned int userId, ClientHandle *remoteHandle);

static int sendPushRequest(unsigned int userID);

static void handleLogin(PClientToLodiServer *request, ClientHandle *clientHandle);

static void handleLogout(PClientToLodiServer *request, ClientHandle *clientHandle);

static void handlePost(PClientToLodiServer *request, ClientHandle *clientHandle);

static void handleFollow(PClientToLodiServer *request, ClientHandle *clientHandle);

static void handleUnfollow(PClientToLodiServer *request, ClientHandle *clientHandle);

static void handleFailure(PClientToLodiServer *request, ClientHandle *clientHandle);

static DomainClient *pkeClient = NULL;
static DomainServer *lodiServer = NULL;
static DomainClient *tfaClient = NULL;

int main() {
  if (initPkeClient(&pkeClient) == ERROR
      || pkeClient->base.start(&pkeClient->base) == ERROR
      || initLodiServer(&lodiServer) == ERROR
      || lodiServer->base.start(&lodiServer->base) == ERROR
      || initTfaClient(&tfaClient) == ERROR
      || tfaClient->base.start(&tfaClient->base) == ERROR) {
    printf("Error: Failed to initialize Lodi Server.\n");
    exit(ERROR);
  }
  initFollowerRepository();
  initListenerRepository();

  while (true) {
    PClientToLodiServer request;
    ClientHandle remoteHandle;
    const int receiveStatus = lodiServer->receive(lodiServer, (UserMessage *) &request, &remoteHandle);

    if (receiveStatus == DOMAIN_FAILURE) {
      printf("Failed to handle incoming PClientToLodiServer message.\n");
      continue;
    }
    if (receiveStatus == TERMINATED) {
      printf("Connection terminated for userId=%d, socket %d\n",
             remoteHandle.userID, remoteHandle.clientSock);
      removeListener(&remoteHandle);
      continue;
    }

    if (request.messageType == login) {
      handleLogin(&request, &remoteHandle);
    } else if (authenticate(&request) == ERROR || !isUserLoggedIn(&remoteHandle)) {
      printf("Authentication failed for userId=%d", request.userID);
      handleFailure(&request, &remoteHandle);
    } else if (request.messageType == logout) {
      handleLogout(&request, &remoteHandle);
    } else if (request.messageType == post) {
      handlePost(&request, &remoteHandle);
    } else if (request.messageType == follow) {
      handleFollow(&request, &remoteHandle);
    } else if (request.messageType == unfollow) {
      handleUnfollow(&request, &remoteHandle);
    } else if (request.messageType == feed) {
      handleFeed(request.userID, &remoteHandle);
    } else {
      printf("Unrecognized request message type, messageType=%d, userId=%d",
             request.messageType, request.userID);
      handleFailure(&request, &remoteHandle);
    }
  }
}

static int authenticate(PClientToLodiServer *request) {
  unsigned int publicKey;
  if (getPublicKey(pkeClient, request->userID, &publicKey) == ERROR) {
    printf("[ERROR] Failed to retrieve public key!\n");
    return ERROR;
  }
  const unsigned long decrypted = decryptTimestamp(request->digitalSig, publicKey, MODULUS);
  if (decrypted == request->timestamp) {
    printf("[DEBUG] Decrypted timestamp successfully! timestamp=%lu \n", decrypted);
    return SUCCESS;
  }
  printf("[ERROR] Failed to decrypt timestamp! timestamp=%lu, decrypted=%lu \n",
         request->timestamp, decrypted);
  return ERROR;
}

static void handleLogin(PClientToLodiServer *request, ClientHandle *clientHandle) {
  printf("[LODI_SERVER] attempting to login user...\n");

  LodiServerMessage responseMessage = {
    .userID = request->userID
  };
  if (authenticate(request) == SUCCESS) {
    responseMessage.messageType = ackLogin;
  } else {
    responseMessage.messageType = failure;
  }
  if (responseMessage.messageType == ackLogin && sendPushRequest(request->userID) == ERROR) {
    printf("[ERROR] Failed to authenticate with push confirmation!\n");
    responseMessage.messageType = failure;
  } else if (responseMessage.messageType == ackLogin) {
    printf("[DEBUG] Validated TFA successfully!\n");
  }

  if (responseMessage.messageType == ackLogin && isUserLoggedIn(clientHandle)) {
    printf("[WARNING] User is already logged in, invalidating previous session.\n");
    userLogout(clientHandle);
  }

  if (responseMessage.messageType == ackLogin) {
    userLogin(clientHandle);
  } else {
    printf("[ERROR] Login failed!\n");
  }

  if (lodiServer->send(lodiServer, (UserMessage *) &responseMessage, clientHandle) == ERROR) {
    printf("[WARMING] Error while sending Lodi login response.\n");
  }
}

static void handleLogout(PClientToLodiServer *request, ClientHandle *clientHandle) {
  printf("[DEBUG] logging out user with userId=%u\n", request->userID);

  LodiServerMessage responseMessage = {
    .userID = request->userID
  };
  if (authenticate(request) == SUCCESS) {
    responseMessage.messageType = ackLogout;
  } else {
    printf("[ERROR] User with userId=%u, failed authentication\n", request->userID);
    responseMessage.messageType = failure;
  }

  if (isUserLoggedIn(clientHandle)) {
    userLogout(clientHandle);
  } else {
    printf("[WARNING] User with userId=%u was already logged out\n", request->userID);
  }

  if (lodiServer->send(lodiServer, (UserMessage *) &responseMessage, clientHandle) == ERROR) {
    printf("[WARNING] Error while sending Lodi logout response for userId=%u.\n", request->userID);
  }
}

static void handlePost(PClientToLodiServer *request, ClientHandle *clientHandle) {
  printf("[DEBUG] Persisting idol message, message=%s...\n", request->message);
  LodiServerMessage responseMessage = {
    .messageType = ackPost,
    .userID = request->userID,
  };
  if (addMessage(request->userID, request->message) == ERROR) {
    responseMessage.messageType = failure;
  } else {
    pushFeedMessage(request->userID, request->message);
  }
  if (lodiServer->send(lodiServer, (UserMessage *) &responseMessage, clientHandle) == ERROR) {
    printf("[WARNING] Error while sending Lodi persistence response.\n");
  }
}

static void handleFollow(PClientToLodiServer *request, ClientHandle *clientHandle) {
  printf("[DEBUG] Handling follow request.\n");
  LodiServerMessage responseMessage = {
    .messageType = ackFollow,
    .userID = request->userID,
  };
  if (addFollower(request->recipientID, request->userID) == ERROR) {
    responseMessage.messageType = failure;
  }
  if (lodiServer->send(lodiServer, (UserMessage *) &responseMessage, clientHandle) == ERROR) {
    printf("[WARNING] Error while sending Lodi follow response.\n");
  }
}

static void handleUnfollow(PClientToLodiServer *request, ClientHandle *clientHandle) {
  printf("[DEBUG] Handling unfollow request.\n");
  LodiServerMessage responseMessage = {
    .messageType = ackUnfollow,
    .userID = request->userID,
  };
  if (removeFollower(request->recipientID, request->userID) == ERROR) {
    responseMessage.messageType = failure;
  }
  if (lodiServer->send(lodiServer, (UserMessage *) &responseMessage, clientHandle) == ERROR) {
    printf("[WARNING] Error while sending Lodi unfollow response.\n");
  }
}

static void handleFailure(PClientToLodiServer *request, ClientHandle *clientHandle) {
  LodiServerMessage responseMessage = {
    .messageType = failure,
    .userID = request->userID,
  };
  if (lodiServer->send(lodiServer, (UserMessage *) &responseMessage, clientHandle) == ERROR) {
    printf("[WARNING] Error while sending Lodi failure.\n");
  }
}

static void handleFeed(const unsigned int userId, ClientHandle *remoteHandle) {
  printf("[DEBUG] Handling feed subscription request.\n");
  addListener(remoteHandle);
  List *idols;
  if (getFollowerIdols(userId, &idols) != SUCCESS) {
    return;
  }
  LodiServerMessage responseMessage = {
    .messageType = ackFeed,
    .userID = userId
  };
  for (int i = 0; i < idols->length; i++) {
    int *idolId;
    idols->get(idols, i, (void **) &idolId);
    responseMessage.recipientID = *idolId;
    List *messages;
    if (getMessages(*idolId, &messages) != SUCCESS) {
      continue;
    }
    for (int j = 0; j < messages->length; j++) {
      char *message = NULL;
      messages->get(messages, j, (void **) &message);
      memcpy(responseMessage.message, message, LODI_MESSAGE_LENGTH * sizeof(char));
      if (lodiServer->send(lodiServer, (UserMessage *) &responseMessage, remoteHandle) == ERROR) {
        printf("[WARNING] Error while responding to initial feed request. Continuing...\n");
      }
    }
  }
}

/**
 * Send a push request to the TFA server
 * @param userID user to authenticate
 * @return ERROR or SUCCESS
 */
static int sendPushRequest(const unsigned int userID) {
  printf("[DEBUG] Sending push request to TFA server\n");
  const TFAClientOrLodiServerToTFAServer requestMessage = {
    .messageType = requestAuth,
    .userID = userID
  };

  if (tfaClient->send(tfaClient, (UserMessage *) &requestMessage) == ERROR) {
    printf("[ERROR] Unable to send push notification, aborting...\n");
    return ERROR;
  }

  TFAServerToLodiServer response;
  if (tfaClient->receive(tfaClient, (UserMessage *) &response) == ERROR) {
    printf("[ERROR] Unable to receive push notification response, aborting...\n");
    return ERROR;
  }

  printf("[DEBUG] Push auth confirmation received! Received: messageType=%u, userID=%u\n", response.messageType,
         response.userID);

  return SUCCESS;
}

static void pushFeedMessage(const unsigned int idolId, char *message) {
  printf("[DEBUG] Publishing messages to idol followers\n");
  List *followers;
  if (getIdolFollowers(idolId, &followers) != SUCCESS) {
    return;
  }
  List *listeners;
  getAllListeners(&listeners);
  LodiServerMessage responseMessage = {
    .messageType = ackFeed,
    .recipientID = idolId
  };
  memcpy(responseMessage.message, message,LODI_MESSAGE_LENGTH * sizeof(char));
  printf("[DEBUG] Publishing messages to all logged-in listening idol followers, idolId=%u, followerCount=%d\n",
         idolId, followers->length);
  for (int i = 0; i < followers->length; i++) {
    int *followerId = NULL;
    followers->get(followers, i, (void **) &followerId);

    for (int j = 0; j < listeners->length; j++) {
      ClientHandle *listener;
      listeners->get(listeners, j, (void **) &listener);
      if (isUserLoggedIn(listener) && listener->userID == *followerId) {
        responseMessage.userID = *followerId;
        const int sendStatus = lodiServer->send(lodiServer, (UserMessage *) &responseMessage, listener);
        if (sendStatus != DOMAIN_SUCCESS) {
          printf("[WARNING] Wasn't able to send message to followerId=%d\n", *followerId);
        } else {
          printf("[DEBUG] Pushed messagwe to followerId=%d\n", *followerId);
        }
      }
    }
  }
}
