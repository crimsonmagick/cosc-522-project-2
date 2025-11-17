/**
*  Interface for shared TFA Server+Client functions for serializing/deserializing domain structs in a network-safe way, converting bytes to and from
 * big-endian.
 */

#ifndef COSC522_LODI_TFAMESSAGING_H
#define COSC522_LODI_TFAMESSAGING_H

#define TFA_CLIENT_REQUEST_SIZE (2 * sizeof(uint32_t) + 2 * sizeof(uint64_t))
#define TFA_SERVER_RESPONSE_SIZE (2 * sizeof(uint32_t))
#include <stdbool.h>
#include "domain_datagram.h"

typedef struct {
  enum { registerTFA, ackRegTFA, ackPushTFA, requestAuth } messageType; /* same size as an unsigned int */
  unsigned int userID; /* user identifier */
  unsigned long timestamp; /* timestamp */
  unsigned long digitalSig; /* encrypted timestamp */
} TFAClientOrLodiServerToTFAServer;

typedef struct {
  enum { confirmTFA, pushTFA } messageType; /* same as unsigned int */
  unsigned int userID; /* user identifier*/
} TFAServerToTFAClient;

typedef struct {
  enum { responseAuth } messageType; /* same size as an unsigned int */
  unsigned int userID; /* user's identifier or requested user identifier*/
} TFAServerToLodiServer;

int initTFAClientDomain(DomainService **service, const bool isDuplex);

int initTFAServerDomain(DomainService **service);

#endif
