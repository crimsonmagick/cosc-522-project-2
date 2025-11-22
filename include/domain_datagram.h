/**
*  WIP service interface for managing interactions between clients and servers while maintaining an open socket and
*  abstracting serialization and deserialization.
 */

#ifndef COSC522_LODI_DOMAIN_DATAGRAM_H
#define COSC522_LODI_DOMAIN_DATAGRAM_H

#include <netinet/in.h>
#include "domain.h"
#include "messaging/network.h"

typedef struct DomainServiceOpts {
  int localPort; // optional
  int receiveTimeoutMs; // optional
  enum ConnectionType connectionType; // required
  MessageSerializer outgoingSerializer; // required
  MessageDeserializer incomingDeserializer; // required
} DomainServiceOpts;

typedef struct DomainClientOpts {
  DomainServiceOpts baseOpts;
  int remotePort;
  char * remoteHost;
} DomainClientOpts;

typedef struct DomainService {
  int sock;
  enum ConnectionType connectionType;
  struct sockaddr_in localAddr;
  struct timeval sendTimeout;
  struct timeval receiveTimeout;

  MessageSerializer outgoingSerializer;
  MessageDeserializer incomingDeserializer;

  int (* start)(struct DomainService *);
  int (* stop)(struct DomainService *);
  int (* destroy)(struct DomainService **);
  int (* changeTimeout)(struct DomainService *, int timeoutMs);
} DomainService;

typedef struct DomainClient {
  DomainService base;
  struct sockaddr_in remoteAddr;
  int (* send)(struct DomainClient *, UserMessage*);
  int (* receive)(struct DomainClient *, UserMessage*);
} DomainClient;

typedef struct DomainHandle {
  unsigned int userID;
  struct sockaddr_in host; // abstract with userID maps?
} DomainHandle;

typedef struct DomainServer {
  DomainService base;
  int (* send)(struct DomainServer *self, UserMessage*, DomainHandle*);
  int (* receive)(struct DomainServer *self, UserMessage*, DomainHandle*);

} DomainServer;

int createClient(DomainClientOpts options, DomainClient** client);
int createServer(DomainServiceOpts options, DomainServer** server);

#endif
