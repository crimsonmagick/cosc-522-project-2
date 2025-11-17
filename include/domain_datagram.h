/**
*  WIP service interface for managing interactions between clients and servers while maintaining an open socket and
*  abstracting serialization and deserialization.
 */

#ifndef COSC522_LODI_DOMAIN_DATAGRAM_H
#define COSC522_LODI_DOMAIN_DATAGRAM_H

#include <netinet/in.h>
#include "domain.h"

typedef struct DatagramDomainService DatagramDomainService;

typedef struct DatagramDomainServiceHandle {
  DatagramDomainService *domainService;
} DatagramDomainServiceHandle;


typedef struct DatagramDomainServiceOpts {
  char *localPort;
  int timeoutMs;
  MessageSerializer outgoingSerializer;
  MessageDeserializer incomingDeserializer;
} DomainServiceOpts;

int startDatagramService(const DomainServiceOpts options, DatagramDomainServiceHandle **handle);

int stopDatagramService(DatagramDomainServiceHandle **handle);

int toDatagramDomainHost(DatagramDomainServiceHandle *handle, void *message, struct sockaddr_in *hostAddr);

int fromDatagramDomainHost(DatagramDomainServiceHandle *handle, void *message, struct sockaddr_in *hostAddr);

int changeDatagramTimeout(DatagramDomainServiceHandle *handle, int timeoutMs);



#endif
