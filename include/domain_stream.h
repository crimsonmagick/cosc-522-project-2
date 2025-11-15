/**
*  WIP service interface for managing interactions between clients and servers while maintaining an open socket and
*  abstracting serialization and deserialization.
 */

#ifndef COSC522_LODI_DOMAIN_STREAM_H
#define COSC522_LODI_DOMAIN_STREAM_H

#include <stdbool.h>
#include <netinet/in.h>
#include "domain.h"

typedef struct StreamDomainService StreamDomainService;

typedef struct StreamDomainServiceHandle {
  StreamDomainService *streamDomainService;
} StreamDomainServiceHandle;

typedef struct StreamDomainServiceOpts {
  char *localPort;
  int timeoutMs;
  char *remotePort;
  struct sockaddr_in remoteHost;
  MessageSerializer outgoingSerializer;
  MessageDeserializer incomingDeserializer;
  bool isServer;
} StreamDomainServiceOpts;

int startStreamService(const StreamDomainServiceOpts options, StreamDomainServiceOpts **handle);

int stopService(StreamDomainServiceHandle **handle);

int toStreamDomainHost(StreamDomainServiceHandle *handle, void *message);

int fromStreamDomainHost(StreamDomainServiceHandle *handle, void *message);

int changeStreamTimeout(StreamDomainServiceHandle *handle, int timeoutMs);

#endif
