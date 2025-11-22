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
  int sendTimeoutMs; // optional
  int receiveTimeoutMs; // optional
  enum ConnectionType connectionType; // required
  MessageSerializer outgoingSerializer; // required
  MessageDeserializer incomingDeserializer; // required
} DomainServiceOpts;

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
} DomainService;

typedef struct DomainClient {
  DomainService base;
  int (* send)(UserMessage*);
  int (* recv)(UserMessage*);
} DomainClient;

typedef struct DomainHandle {
  unsigned int userID;
  struct sockaddr_in host; // abstract with userID maps?
} DomainHandle;

typedef struct DomainServer {
  DomainService base;
  int (* send)(struct DomainServer self, UserMessage*, DomainHandle*);
  int (* receive)(struct DomainServer self, UserMessage*, DomainHandle*);

} DomainServer;

int createServer(DomainServiceOpts options, DomainServer** server);
int createClient(DomainServiceOpts options, DomainClient** client);

int startDatagramService(const DomainServiceOpts options, DomainService **service);

int stopDatagramService(DomainService **service);

int toDatagramDomainHost(DomainService *service, void *message, struct sockaddr_in *hostAddr);

int fromDatagramDomainHost(DomainService *service, void *message, struct sockaddr_in *hostAddr);

int changeDatagramTimeout(DomainService *service, int timeoutMs);



#endif
