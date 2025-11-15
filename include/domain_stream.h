// /**
// *  WIP service interface for managing interactions between clients and servers while maintaining an open socket and
// *  abstracting serialization and deserialization.
//  */
//
// #ifndef COSC522_LODI_DOMAIN_STREAM_H
// #define COSC522_LODI_DOMAIN_STREAM_H
//
// #include <netinet/in.h>
// #include "domain.h"
//
// typedef struct StreamDomainService StreamDomainService;
//
// typedef struct DomainServiceHandle {
//   DomainService *domainService;
// } DomainServiceHandle;
//
// typedef struct MessageSerializer {
//   size_t messageSize;
//
//   int (*serializer)(void *, char *);
// } MessageSerializer;
//
// typedef struct MessageDeserializer {
//   size_t messageSize;
//
//   int (*deserializer)(char *, void *);
// } MessageDeserializer;
//
//
// typedef struct DomainServiceOpts {
//   char *localPort;
//   int timeoutMs;
//   MessageSerializer outgoingSerializer;
//   MessageDeserializer incomingDeserializer;
// } DomainServiceOpts;
//
// int startService(const DomainServiceOpts options, DomainServiceHandle **handle);
//
// int stopService(DomainServiceHandle **handle);
//
// int toDomainHost(DomainServiceHandle *handle, void *message, struct sockaddr_in *hostAddr);
//
// int fromDomainHost(DomainServiceHandle *handle, void *message, struct sockaddr_in *hostAddr);
//
// int changeTimeout(DomainServiceHandle *handle, int timeoutMs);
//
//
//
// #endif
