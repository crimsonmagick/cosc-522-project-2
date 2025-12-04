/**
 * Manages interactions with the Lodi Server.
 */

#ifndef COSC522_LODI_LODI_SERVICE_H
#define COSC522_LODI_LODI_SERVICE_H
#include "domain/lodi.h"
#include "domain/pke.h"

int lodiClientSend(const PClientToLodiServer *inRequest, LodiServerMessage *outResponse);

int lodiClientPkeSend(const PClientToPKServer *inRequest, PKServerToLodiClient *responseOut);

#endif
