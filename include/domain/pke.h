/**
*  Interface for Shared PKE Server+Client functions for serializing/deserializing domain structs in a network-safe way, converting bytes to and from
 * big-endian.
 */

#ifndef COSC522_LODI_PKEMESSAGING_H
#define COSC522_LODI_PKEMESSAGING_H

#define PK_CLIENT_REQUEST_SIZE (3 * sizeof(uint32_t))
#define PK_SERVER_RESPONSE_SIZE (3 * sizeof(uint32_t))
#include "domain/domain.h"

typedef struct {
  enum { ackRegisterKey, responsePublicKey } messageType; /* same as unsigned int */
  unsigned int userID; /* user identifier or user identifier of requested public key*/
  unsigned int publicKey; /* registered public key or requested public key */
} PKServerToLodiClient;

typedef PKServerToLodiClient PKServerToPClientOrLodiServer;

typedef struct {
  enum { registerKey, requestKey } messageType; /* same size as an unsigned int */
  unsigned int userID; /* user's identifier or requested user identifier*/
  unsigned int publicKey; /* user's public key or 0 if message_type is request_key */
} PClientToPKServer;

/*
 * Constructor functions
 */

int initPkeClient(DomainClient **client);

int initPKEServer(DomainServer **server);

/**
 * Gets the public key for a user for the PKE Server
 *
 * @param client Domain Service to use to retrieve public key
 * @param userID user to retrieve for
 * @param publicKey output, the retrieved public key
 * @return ERROR, SUCCESS
 */
int getPublicKey(DomainClient *client, const unsigned int userID, unsigned int *publicKey);

#endif
