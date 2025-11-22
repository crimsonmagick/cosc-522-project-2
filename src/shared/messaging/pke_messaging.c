/**
* Shared PKE Server+Client functions for serializing/deserializing domain structs in a network-safe way, converting bytes to and from
 * big-endian.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <netinet/in.h>

#include "messaging/pke_messaging.h"

#include "domain_datagram.h"
#include "shared.h"
#include "util/buffers.h"
#include "util/server_configs.h"

/**
 * Gets the public key for a user for the PKE Server
 *
 * @param service Domain Service to use to retrieve public key
 * @param pkeAddr Domain Service address
 * @param userID user to retrieve for
 * @param publicKey output, the retrieved public key
 * @return ERROR, SUCCESS
 */
int getPublicKey(DomainService *service, struct sockaddr_in *pkeAddr, const unsigned int userID,
                 unsigned int *publicKey) {
  const PClientToPKServer requestMessage = {
    .messageType = requestKey,
    userID
  };

  if (toDatagramDomainHost(service, (void *) &requestMessage, pkeAddr) == DOMAIN_FAILURE) {
    printf("Unable to get public key, aborting ...\n");
    return ERROR;
  }

  PKServerToLodiClient responseMessage;
  struct sockaddr_in receivedAddr;
  if (fromDatagramDomainHost(service, &responseMessage, &receivedAddr) == DOMAIN_FAILURE) {
    printf("Failed to receive public key, aborting ...\n");
    return ERROR;
  }

  printf("Received public key successfully! Received: messageType=%u, userID=%u, publicKey=%u\n",
         responseMessage.messageType, responseMessage.userID, responseMessage.publicKey);
  *publicKey = responseMessage.publicKey;

  return SUCCESS;
}

/*
 * Boilerplate serdes functions
 */

int serializeClientPK(PClientToPKServer *toSerialize, char *serialized) {
  size_t offset = 0;
  appendUint32(serialized, &offset, toSerialize->messageType);
  appendUint32(serialized, &offset, toSerialize->userID);
  appendUint32(serialized, &offset, toSerialize->publicKey);

  return MESSAGE_SERIALIZER_SUCCESS;
}

int serializeServerPK(PKServerToLodiClient *toSerialize, char *serialized) {
  size_t offset = 0;
  appendUint32(serialized, &offset, toSerialize->messageType);
  appendUint32(serialized, &offset, toSerialize->userID);
  appendUint32(serialized, &offset, toSerialize->publicKey);

  return MESSAGE_SERIALIZER_SUCCESS;
}

int deserializeClientPK(char *serialized, PKServerToLodiClient *deserialized) {
  size_t offset = 0;
  deserialized->messageType = getUint32(serialized, &offset);
  deserialized->userID = getUint32(serialized, &offset);
  deserialized->publicKey = getUint32(serialized, &offset);

  return MESSAGE_DESERIALIZER_SUCCESS;
}

int deserializeServerPK(char *serialized, PKServerToLodiClient *deserialized) {
  size_t offset = 0;
  deserialized->messageType = getUint32(serialized, &offset);
  deserialized->userID = getUint32(serialized, &offset);
  deserialized->publicKey = getUint32(serialized, &offset);

  return MESSAGE_DESERIALIZER_SUCCESS;
}

int initPKEClientDomain(DomainService **service) {
  const MessageSerializer outgoing = {
    PK_CLIENT_REQUEST_SIZE,
    .serializer = (int (*)(void *, char *)) serializeClientPK
  };
  const MessageDeserializer incoming = {
    PK_SERVER_RESPONSE_SIZE,
    .deserializer = (int (*)(char *, void *)) deserializeServerPK
  };
  const DomainServiceOpts options = {
    .localPort = 0,
    .receiveTimeoutMs = DEFAULT_TIMEOUT_MS,
    .outgoingSerializer = outgoing,
    .incomingDeserializer = incoming
  };

  DomainService *allocatedService = NULL;
  if (startDatagramService(options, &allocatedService) != DOMAIN_SUCCESS) {
    return ERROR;
  }
  *service = allocatedService;
  return SUCCESS;
}

/*
 * Boilerplate DomainService constructor functions
 */

int initPKEServerDomain(DomainService **service) {
  const ServerConfig serverConfig = getServerConfig(PK);
  const MessageSerializer outgoing = {
    PK_SERVER_RESPONSE_SIZE,
    .serializer = (int (*)(void *, char *)) serializeServerPK
  };
  const MessageDeserializer incoming = {
    PK_CLIENT_REQUEST_SIZE,
    .deserializer = (int (*)(char *, void *)) deserializeClientPK
  };
  const DomainServiceOpts options = {
    .localPort = atoi(serverConfig.port),
    .receiveTimeoutMs = 0,
    .outgoingSerializer = outgoing,
    .incomingDeserializer = incoming
  };

  DomainService *allocatedService = NULL;
  if (startDatagramService(options, &allocatedService) != DOMAIN_SUCCESS) {
    return ERROR;
  }
  *service = allocatedService;
  return SUCCESS;
}
