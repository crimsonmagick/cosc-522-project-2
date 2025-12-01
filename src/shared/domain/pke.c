/**
* Shared PKE Server+Client functions for serializing/deserializing domain structs in a network-safe way, converting bytes to and from
 * big-endian.
 */

#include <stdio.h>
#include <stdlib.h>

#include "domain/pke.h"
#include "shared.h"
#include "util/buffers.h"
#include "util/server_configs.h"

/**
 * Gets the public key for a user for the PKE Server
 *
 * @param client Domain Service to use to retrieve public key
 * @param userID user to retrieve for
 * @param publicKey output, the retrieved public key
 * @return ERROR, SUCCESS
 */
int getPublicKey(DomainClient *client, const unsigned int userID, unsigned int *publicKey) {
  const PClientToPKServer requestMessage = {
    .messageType = requestKey,
    userID
  };

  if (client->send(client, (UserMessage *) &requestMessage) == DOMAIN_FAILURE) {
    printf("Unable to get public key, aborting ...\n");
    return ERROR;
  }

  PKServerToLodiClient responseMessage;
  if (client->receive(client, (UserMessage *) &responseMessage) == DOMAIN_FAILURE) {
    printf("[ERROR] Failed to receive public key, aborting ...\n");
    return ERROR;
  }

  printf("[DEBUG] Received public key successfully! Received: messageType=%u, userID=%u, publicKey=%u\n",
         responseMessage.messageType, responseMessage.userID, responseMessage.publicKey);
  *publicKey = responseMessage.publicKey;

  return SUCCESS;
}

/*
 * Boilerplate serdes functions
 */

static int serializeClientPK(PClientToPKServer *toSerialize, char *serialized) {
  size_t offset = 0;
  appendUint32(serialized, &offset, toSerialize->messageType);
  appendUint32(serialized, &offset, toSerialize->userID);
  appendUint32(serialized, &offset, toSerialize->publicKey);

  return MESSAGE_SERIALIZER_SUCCESS;
}

static int serializeServerPK(PKServerToLodiClient *toSerialize, char *serialized) {
  size_t offset = 0;
  appendUint32(serialized, &offset, toSerialize->messageType);
  appendUint32(serialized, &offset, toSerialize->userID);
  appendUint32(serialized, &offset, toSerialize->publicKey);

  return MESSAGE_SERIALIZER_SUCCESS;
}

static int deserializeClientPK(char *serialized, PKServerToLodiClient *deserialized) {
  size_t offset = 0;
  deserialized->messageType = getUint32(serialized, &offset);
  deserialized->userID = getUint32(serialized, &offset);
  deserialized->publicKey = getUint32(serialized, &offset);

  return MESSAGE_DESERIALIZER_SUCCESS;
}

static int deserializeServerPK(char *serialized, PKServerToLodiClient *deserialized) {
  size_t offset = 0;
  deserialized->messageType = getUint32(serialized, &offset);
  deserialized->userID = getUint32(serialized, &offset);
  deserialized->publicKey = getUint32(serialized, &offset);

  return MESSAGE_DESERIALIZER_SUCCESS;
}

int initPkeClient(DomainClient **client) {
  const ServerConfig serverConfig = getServerConfig(PK);
  const MessageSerializer outgoing = {
    PK_CLIENT_REQUEST_SIZE,
    .serializer = (int (*)(void *, char *)) serializeClientPK
  };
  const MessageDeserializer incoming = {
    PK_SERVER_RESPONSE_SIZE,
    .deserializer = (int (*)(char *, void *)) deserializeServerPK
  };
  const DomainClientOpts options = {
    .baseOpts = {
      .localPort = 0,
      .localHost = "10.202.0.2",
      .receiveTimeoutMs = DEFAULT_TIMEOUT_MS,
      .outgoingSerializer = outgoing,
      .incomingDeserializer = incoming,
      .connectionType = DATAGRAM
    },
    .remotePort = atoi(serverConfig.port),
    .remoteHost = serverConfig.address
  };

  if (createClient(options, client) != DOMAIN_SUCCESS) {
    return ERROR;
  }
  printf("Configured PKE client with address=%s, port=%s\n", serverConfig.address, serverConfig.port);
  return SUCCESS;
}

/*
 * Boilerplate DomainService constructor functions
 */

int initPKEServer(DomainServer **server) {
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
    .localHost = serverConfig.address,
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
