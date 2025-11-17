/**
*  WIP service interface for managing interactions between clients and servers while maintaining an open socket and
*  abstracting serialization and deserialization.
 */

#ifndef COSC522_LODI_DOMAIN_DATAGRAM_H
#define COSC522_LODI_DOMAIN_DATAGRAM_H

#include <netinet/in.h>
#include "domain.h"

typedef struct DomainServiceOpts {
  char *localPort;
  int timeoutMs;
  MessageSerializer outgoingSerializer;
  MessageDeserializer incomingDeserializer;
} DomainServiceOpts;

typedef struct DomainService {
  int sock;
  struct sockaddr_in localAddr;
  MessageSerializer outgoingSerializer;
  MessageDeserializer incomingDeserializer;
  int (*start)();
  int (*stop)();
} DomainService;

typedef struct DomainClient {
  DomainService base;
  int (*send)(void *);
  int (*recv)(void *);
} DomainClient;

typedef struct DomainHandle {
  struct sockaddr_in host;
} DomainHandle;

typedef struct DomainServer {
  DomainService base;
  int (*send)(void *, DomainHandle *);
  int (*recv)(void *, DomainHandle *);

} DomainServer;

int startDatagramService(const DomainServiceOpts options, DomainService **service);

int stopDatagramService(DomainService **service);

int toDatagramDomainHost(DomainService *service, void *message, struct sockaddr_in *hostAddr);

int fromDatagramDomainHost(DomainService *service, void *message, struct sockaddr_in *hostAddr);

int changeDatagramTimeout(DomainService *service, int timeoutMs);



#endif
