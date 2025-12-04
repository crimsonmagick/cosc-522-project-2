/*
 * See lodi_client_domain_manager.h
 */

#include "lodi_client_domain_manager.h"

#include <stdio.h>

#include "shared.h"
#include "domain/domain.h"
#include "domain/lodi.h"
#include "domain/pke.h"

static DomainClient *lodiClient = NULL;
static DomainClient *pkeClient = NULL;

int lodiClientSend(const PClientToLodiServer *inRequest, LodiServerMessage *outResponse) {
  int status = SUCCESS;
  if (lodiClient == NULL && initLodiClient(&lodiClient) == ERROR) {
    printf("Failed to initialize Lodi Client\n");
    return ERROR;
  }

  if (lodiClient->base.start(&lodiClient->base) == DOMAIN_FAILURE) {
    printf("Failed to start Lodi Client\n");
    return ERROR;
  };

  if (lodiClient->send(lodiClient, (UserMessage *) inRequest) != DOMAIN_SUCCESS) {
    printf("Failed to send from Lodi Client\n");
    status = ERROR;
  }

  if (status == SUCCESS && lodiClient->receive(lodiClient, (UserMessage *) outResponse) != DOMAIN_SUCCESS) {
    printf("Failed to receive from Lodi Client\n");
    status = ERROR;
  }

  if (lodiClient->base.stop(&lodiClient->base) == DOMAIN_FAILURE) {
    printf("Failed to stop Lodi Client\n");
    status = ERROR;
  }
  return status;
}

int lodiClientPkeSend(const PClientToPKServer *inRequest, PKServerToLodiClient *responseOut) {
  if (pkeClient == NULL) {
    initPkeClient(&pkeClient);
  }

  pkeClient->base.start(&pkeClient->base);
  if (pkeClient->send(pkeClient, (UserMessage *) inRequest) == DOMAIN_FAILURE) {
    return ERROR;
  }

  if (pkeClient->receive(pkeClient, (UserMessage *) responseOut) == DOMAIN_FAILURE) {
    return ERROR;
  }
  pkeClient->base.stop(&pkeClient->base);

  return SUCCESS;
}
