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
    status = ERROR;
  }

  if (status == SUCCESS && lodiClient->receive(lodiClient, (UserMessage *) outResponse) == DOMAIN_FAILURE) {
    status = ERROR;
  }

  lodiClient->base.stop(&lodiClient->base);
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
