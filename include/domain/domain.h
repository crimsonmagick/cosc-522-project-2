/**
 *  Services for managing interactions between clients and servers while maintaining an open socket and abstracting
 *  serialization and deserialization.
 */


#ifndef COSC522_LODI_DOMAIN_H
#define COSC522_LODI_DOMAIN_H
#include <stdbool.h>

#include "collections/list.h"

#define DOMAIN_SUCCESS 0
#define MESSAGE_SERIALIZER_SUCCESS 0
#define MESSAGE_SERIALIZER_FAILURE 1
#define MESSAGE_DESERIALIZER_SUCCESS 0
#define MESSAGE_DESERIALIZER_FAILURE 1

#define DOMAIN_FAILURE 1

#define DEFAULT_TIMEOUT_MS 500

typedef struct MessageSerializer {
  size_t messageSize;

  int (*serializer)(void *, char *);
} MessageSerializer;

typedef struct MessageDeserializer {
  size_t messageSize;

  int (*deserializer)(char *, void *);
} MessageDeserializer;

typedef struct {
  unsigned int messageType; /* placeholder for implementations */
  unsigned int userID; /* user identifier, common to all messages*/
  // struct may have arbitrary fields contiguously in memory after the userID
} UserMessage;

#include <netinet/in.h>
#include "domain/domain.h"
#include "../util/network.h"

typedef struct DomainServiceOpts {
  int localPort; // optional
  char *localHost;
  int receiveTimeoutMs; // optional
  enum ConnectionType connectionType; // required
  MessageSerializer outgoingSerializer; // required
  MessageDeserializer incomingDeserializer; // required
} DomainServiceOpts;

typedef struct DomainClientOpts {
  DomainServiceOpts baseOpts;
  int remotePort;
  char *remoteHost;
} DomainClientOpts;

typedef struct DomainService {
  int sock;
  enum ConnectionType connectionType;
  struct sockaddr_in localAddr;
  struct timeval receiveTimeout;

  MessageSerializer outgoingSerializer;
  MessageDeserializer incomingDeserializer;

  int (*start)(struct DomainService *);

  int (*stop)(struct DomainService *);

  int (*destroy)(struct DomainService **);

  int (*changeTimeout)(struct DomainService *, int timeoutMs);
} DomainService;

typedef struct DomainClient {
  DomainService base;
  struct sockaddr_in remoteAddr;
  bool isConnected;

  int (*send)(struct DomainClient *, UserMessage *);

  int (*receive)(struct DomainClient *, UserMessage *);
} DomainClient;

typedef struct ClientHandle {
  unsigned int userID;
  struct sockaddr_in clientAddr;
  int clientSock;
} ClientHandle;

typedef struct DomainServer {
  DomainService base;
  List *clients;

  int (*send)(struct DomainServer *self, UserMessage *, ClientHandle *);

  int (*receive)(struct DomainServer *self, UserMessage *, ClientHandle *);
} DomainServer;

int createClient(DomainClientOpts options, DomainClient **client);

int createServer(DomainServiceOpts options, DomainServer **server);


#endif
