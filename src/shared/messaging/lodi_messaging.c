/**
 * Shared Lodi Server+Client functions for serializing/deserializing domain structs in a network-safe way, converting bytes to and from
 * big-endian.
 */

#include <stdio.h>
#include <string.h>

#include "messaging/lodi_messaging.h"

#include "shared.h"
#include "util/buffers.h"
#include "util/server_configs.h"
#include "domain_datagram.h"
#include "domain_stream.h"

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
  memcpy(deserialized->message, serialized + offset, LODI_MESSAGE_LENGTH * sizeof(char));

  return MESSAGE_DESERIALIZER_SUCCESS;
}

/*
 * Boilerplate DomainService constructor functions
 */

int initLodiClientDomain(StreamDomainServiceHandle **handle) {
  const MessageSerializer outgoing = {
    LODI_CLIENT_REQUEST_SIZE,
    .serializer = (int (*)(void *, char *)) serializeClientLodi
  };
  const MessageDeserializer incoming = {
    LODI_SERVER_RESPONSE_SIZE,
    .deserializer = (int (*)(char *, void *)) deserializeServerLoginLodi
  };

  char * remotePort = getServerConfig(LODI).port;
  struct sockaddr_in remote = getServerAddr(LODI);

  const StreamDomainServiceOpts options = {
    .localPort = 0,
    .timeoutMs = DEFAULT_TIMEOUT_MS,
    .remotePort = remotePort,
    .remoteHost = remote,
    .outgoingSerializer = outgoing,
    .incomingDeserializer = incoming,
    .isServer = false
  };

  StreamDomainServiceHandle *allocatedHandle = NULL;
  if (startStreamService(options, &allocatedHandle) != DOMAIN_SUCCESS) {
    return ERROR;
  }
  *handle = allocatedHandle;
  return SUCCESS;
}

int initLodiServerDomain(StreamDomainServiceHandle **handle) {
  const ServerConfig serverConfig = getServerConfig(LODI);
  const MessageSerializer outgoing = {
    LODI_SERVER_RESPONSE_SIZE,
    .serializer = (int (*)(void *, char *)) serializeServerLoginLodi
  };
  const MessageDeserializer incoming = {
    LODI_CLIENT_REQUEST_SIZE,
    .deserializer = (int (*)(char *, void *)) deserializeClientLodi
  };

  const StreamDomainServiceOpts options = {
    .localPort = serverConfig.port,
    .timeoutMs = 1000 * 60 * 10,
    .outgoingSerializer = outgoing,
    .incomingDeserializer = incoming,
    .isServer = true
  };

  StreamDomainServiceHandle *allocatedHandle = NULL;
  if (startStreamService(options, &allocatedHandle) != DOMAIN_SUCCESS) {
    return ERROR;
  }
  *handle = allocatedHandle;
  return SUCCESS;
}
