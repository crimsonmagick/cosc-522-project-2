/**
* Shared TFA Server+Client functions for serializing/deserializing domain structs in a network-safe way, converting bytes to and from
 * big-endian.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "domain/domain.h"
#include "../../../include/domain/tfa.h"
#include "shared.h"
#include "util/buffers.h"
#include "util/server_configs.h"

/*
 * Boilerplate serdes functions
 */

int serializeClientTFA(TFAClientOrLodiServerToTFAServer *toSerialize, char *serialized) {
  size_t offset = 0;
  appendUint32(serialized, &offset, toSerialize->messageType);
  appendUint32(serialized, &offset, toSerialize->userID);
  appendUint64(serialized, &offset, toSerialize->timestamp);
  appendUint64(serialized, &offset, toSerialize->digitalSig);

  return MESSAGE_SERIALIZER_SUCCESS;
}

int serializeServerTFA(TFAServerToTFAClient *toSerialize, char *serialized) {
  size_t offset = 0;
  appendUint32(serialized, &offset, toSerialize->messageType);
  appendUint32(serialized, &offset, toSerialize->userID);

  return MESSAGE_SERIALIZER_SUCCESS;
}

int deserializeClientTFA(char *serialized, TFAClientOrLodiServerToTFAServer *deserialized) {
  size_t offset = 0;
  deserialized->messageType = getUint32(serialized, &offset);
  deserialized->userID = getUint32(serialized, &offset);
  deserialized->timestamp = getUint64(serialized, &offset);
  deserialized->digitalSig = getUint64(serialized, &offset);

  return MESSAGE_DESERIALIZER_SUCCESS;
}

int deserializeServerTFA(char *serialized, TFAServerToLodiServer *deserialized) {
  size_t offset = 0;
  deserialized->messageType = getUint32(serialized, &offset);
  deserialized->userID = getUint32(serialized, &offset);

  return MESSAGE_DESERIALIZER_SUCCESS;
}

/*
 * Boilerplate DomainService constructor functions
 */

int initTFAClientDomain(DomainClient **client, const bool bindToPort) {
  ServerConfig serverConfig = getServerConfig(TFA);
  const MessageSerializer outgoing = {
    TFA_CLIENT_REQUEST_SIZE,
    .serializer = (int (*)(void *, char *))serializeClientTFA
  };
  const MessageDeserializer incoming = {
    TFA_SERVER_RESPONSE_SIZE,
    .deserializer = (int (*)(char *, void *)) deserializeServerTFA
  };
  DomainClientOpts options = {
    .baseOpts = {
      .localPort = 0,
      .receiveTimeoutMs = DEFAULT_TIMEOUT_MS,
      .outgoingSerializer = outgoing,
      .incomingDeserializer = incoming,
      .connectionType = DATAGRAM
    },
    .remotePort = atoi(serverConfig.port),
    .remoteHost = serverConfig.address
  };
  // if (bindToPort) {
  //   const ServerConfig server_config = getServerConfig(TFA_CLIENT);
  //   options.baseOpts.localPort = atoi(server_config.port);
  // }

  if (createClient(options, client) != DOMAIN_SUCCESS) {
    return ERROR;
  }
  return SUCCESS;
}

int initTFAServerDomain(DomainServer **server) {
  const ServerConfig serverConfig = getServerConfig(TFA);
  const MessageSerializer outgoing = {
    TFA_SERVER_RESPONSE_SIZE,
    .serializer = (int (*)(void *, char *)) serializeServerTFA
  };
  const MessageDeserializer incoming = {
    TFA_CLIENT_REQUEST_SIZE,
    .deserializer = (int (*)(char *, void *)) deserializeClientTFA
  };
  const DomainServiceOpts options = {
    .localPort = atoi(serverConfig.port),
    .receiveTimeoutMs = 0,
    .outgoingSerializer = outgoing,
    .incomingDeserializer = incoming,
    .connectionType = DATAGRAM
  };

  if (createServer(options, server) != DOMAIN_SUCCESS) {
    return ERROR;
  }
  return SUCCESS;
}