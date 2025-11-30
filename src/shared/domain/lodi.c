/**
 * Shared Lodi Server+Client functions for serializing/deserializing domain structs in a network-safe way, converting bytes to and from
 * big-endian.
 */

#include <stdlib.h>
#include <string.h>

#include "domain/lodi.h"
#include "shared.h"
#include "util/buffers.h"
#include "util/server_configs.h"

/*
 * Boilerplate serdes functions
 */

int serializeClientLodi(PClientToLodiServer *toSerialize, char *serialized) {
  size_t offset = 0;
  appendUint32(serialized, &offset, toSerialize->messageType);
  appendUint32(serialized, &offset, toSerialize->userID);
  appendUint32(serialized, &offset, toSerialize->recipientID);
  appendUint64(serialized, &offset, toSerialize->timestamp);
  appendUint64(serialized, &offset, toSerialize->digitalSig);
  memcpy(serialized + offset, toSerialize->message, LODI_MESSAGE_LENGTH * sizeof(char));
  return MESSAGE_SERIALIZER_SUCCESS;
}

int serializeServerLoginLodi(LodiServerMessage *toSerialize, char *serialized) {
  size_t offset = 0;
  appendUint32(serialized, &offset, toSerialize->messageType);
  appendUint32(serialized, &offset, toSerialize->userID);
  appendUint32(serialized, &offset, toSerialize->recipientID);
  memcpy(serialized + offset, toSerialize->message, LODI_MESSAGE_LENGTH * sizeof(char));

  return MESSAGE_SERIALIZER_SUCCESS;
}

int deserializeClientLodi(char *serialized, PClientToLodiServer *deserialized) {
  size_t offset = 0;
  deserialized->messageType = getUint32(serialized, &offset);
  deserialized->userID = getUint32(serialized, &offset);
  deserialized->recipientID = getUint32(serialized, &offset);
  deserialized->timestamp = getUint64(serialized, &offset);
  deserialized->digitalSig = getUint64(serialized, &offset);
  memcpy(deserialized->message, serialized + offset, LODI_MESSAGE_LENGTH * sizeof(char));

  return MESSAGE_DESERIALIZER_SUCCESS;
}

int deserializeServerLoginLodi(char *serialized, LodiServerMessage *deserialized) {
  size_t offset = 0;
  deserialized->messageType = getUint32(serialized, &offset);
  deserialized->userID = getUint32(serialized, &offset);
  deserialized->recipientID= getUint32(serialized, &offset);
  memcpy(deserialized->message, serialized + offset, LODI_MESSAGE_LENGTH * sizeof(char));

  return MESSAGE_DESERIALIZER_SUCCESS;
}

/*
 * Boilerplate DomainService constructor functions
 */

int initLodiClient(DomainClient **domainClient) {
  const MessageSerializer outgoing = {
    LODI_CLIENT_REQUEST_SIZE,
    .serializer = (int (*)(void *, char *)) serializeClientLodi
  };
  const MessageDeserializer incoming = {
    LODI_SERVER_RESPONSE_SIZE,
    .deserializer = (int (*)(char *, void *)) deserializeServerLoginLodi
  };

  char *remotePort = getServerConfig(LODI).port;
  char *remote = getServerConfig(LODI).address;

  const DomainClientOpts options = {
    .baseOpts = {
      .localPort = -1,
      .receiveTimeoutMs = 0,
      .connectionType = STREAM,
      .outgoingSerializer = outgoing,
      .incomingDeserializer = incoming
    },
    .remotePort = atoi(remotePort),
    .remoteHost = remote
  };

  if (createClient(options, domainClient) != DOMAIN_SUCCESS) {
    return ERROR;
  }
  return SUCCESS;
}

int initLodiServerDomain(DomainServer **server) {
  const ServerConfig serverConfig = getServerConfig(LODI);
  const MessageSerializer outgoing = {
    LODI_SERVER_RESPONSE_SIZE,
    .serializer = (int (*)(void *, char *)) serializeServerLoginLodi
  };
  const MessageDeserializer incoming = {
    LODI_CLIENT_REQUEST_SIZE,
    .deserializer = (int (*)(char *, void *)) deserializeClientLodi
  };

  const DomainServiceOpts options = {
    .localPort = atoi(serverConfig.port),
    .receiveTimeoutMs = 0,
    .outgoingSerializer = outgoing,
    .incomingDeserializer = incoming,
    .connectionType = STREAM
  };

  if (createServer(options, server) != DOMAIN_SUCCESS) {
    return ERROR;
  }
  return SUCCESS;
}