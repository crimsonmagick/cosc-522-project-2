/**
*  WIP service interface for managing interactions between clients and servers while maintaining an open socket and
*  abstracting serialization and deserialization.
 */

#ifndef COSC522_LODI_DOMAIN_DATAGRAM_H
#define COSC522_LODI_DOMAIN_DATAGRAM_H

#include <netinet/in.h>
#include "domain.h"

typedef struct DatagramDomainService DatagramDomainService;

typedef struct DatagramDomainServiceOpts {
  char *localPort;
  int timeoutMs;
  MessageSerializer outgoingSerializer;
  MessageDeserializer incomingDeserializer;
} DomainServiceOpts;

int startDatagramService(const DomainServiceOpts options, DatagramDomainService **service);

int stopDatagramService(DatagramDomainService **service);

int toDatagramDomainHost(DatagramDomainService *service, void *message, struct sockaddr_in *hostAddr);

int fromDatagramDomainHost(DatagramDomainService *service, void *message, struct sockaddr_in *hostAddr);

int changeDatagramTimeout(DatagramDomainService *service, int timeoutMs);



#endif
