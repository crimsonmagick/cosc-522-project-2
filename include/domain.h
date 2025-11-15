/**
*  WIP service interface for managing interactions between clients and servers while maintaining an open socket and
*  abstracting serialization and deserialization.
 */

#ifndef COSC522_LODI_DOMAIN_H
#define COSC522_LODI_DOMAIN_H

#define DOMAIN_SUCCESS 0
#define MESSAGE_SERIALIZER_SUCCESS 0
#define MESSAGE_SERIALIZER_FAILURE 1
#define MESSAGE_DESERIALIZER_SUCCESS 0
#define MESSAGE_DESERIALIZER_FAILURE 1
#define DOMAIN_FAILURE 1
#define DOMAIN_INIT_FAILURE 2

#define DEFAULT_TIMEOUT_MS 0

#include <netinet/in.h>
typedef struct DomainService DomainService;

typedef struct DomainServiceHandle {
  DomainService *domainService;
} DomainServiceHandle;

typedef struct MessageSerializer {
  size_t messageSize;

  int (*serializer)(void *, char *);
} MessageSerializer;

typedef struct MessageDeserializer {
  size_t messageSize;

  int (*deserializer)(char *, void *);
} MessageDeserializer;


typedef struct DomainServiceOpts {
  char *localPort;
  int timeoutMs;
  MessageSerializer outgoingSerializer;
  MessageDeserializer incomingDeserializer;
} DomainServiceOpts;

int startService(const DomainServiceOpts options, DomainServiceHandle **handle);

int stopService(DomainServiceHandle **handle);

int toDomainHost(DomainServiceHandle *handle, void *message, struct sockaddr_in *hostAddr);

int fromDomainHost(DomainServiceHandle *handle, void *message, struct sockaddr_in *hostAddr);

int changeTimeout(DomainServiceHandle *handle, int timeoutMs);



#endif
