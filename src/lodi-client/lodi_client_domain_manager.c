#include "lodi_client_domain_manager.h"

#include <stdio.h>

#include "shared.h"
#include "domain/domain.h"
#include "domain/lodi.h"
#include "domain/pke.h"

static DomainClient *lodiClient = NULL;
static DomainClient *pkeClient = NULL;

int lodiClientSend(const PClientToLodiServer *inRequest, LodiServerMessage *outResponse) {
  if (lodiClient == NULL) {
    initLodiClient(&lodiClient);
  }

  lodiClient->base.start(&lodiClient->base);

  int status = SUCCESS;
  if (lodiClient->send(lodiClient, (UserMessage *) inRequest) == ERROR) {
    printf("Failed to send to Lodi server ...\n");
    status = ERROR;
  }

  if (status == SUCCESS && lodiClient->receive(lodiClient, (UserMessage *) outResponse) == DOMAIN_FAILURE) {
    printf("Failed to receive Lodi response...\n");
    status = ERROR;
  } else {
    printf("Received response from Lodi server: messageType=%u, userID=%u\n",
           outResponse->messageType, outResponse->userID);
  }

  lodiClient->base.stop(&lodiClient->base);
  return status;
}

int lodiClientPkeSend(const PClientToPKServer *inRequest, PKServerToLodiClient *responseOut) {
  if (pkeClient == NULL) {
    initPkeClient(&pkeClient);
  }

  pkeClient->base.start(&pkeClient->base);
  if (pkeClient->send(pkeClient, (UserMessage *) &inRequest) == DOMAIN_FAILURE) {
    printf("Unable to send registration, aborting ...\n");
    return ERROR;
  }
  printf("Req A. 1. a. - sent registerKey Message to PKE Server\n");

  if (pkeClient->receive(pkeClient, (UserMessage *) responseOut) == DOMAIN_FAILURE) {
    printf("Failed to receive registration confirmation, aborting ...\n");
    return ERROR;
  }
  printf("Message details: messageType=%u, userID=%u, publicKey=%u\n",
         responseOut->messageType, responseOut->userID, responseOut->publicKey);
  pkeClient->base.stop(&pkeClient->base);

  return SUCCESS;
}
