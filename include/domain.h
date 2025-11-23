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
#include <stdbool.h>

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
  bool isConnected;
  int (* send)(struct DomainClient *, UserMessage*);
  int (* receive)(struct DomainClient *, UserMessage*);
} DomainClient;

typedef struct DomainHandle {
  unsigned int userID;
  struct sockaddr_in host; // abstract with userID maps?
  int clientSock;
} DomainHandle;

typedef struct DomainServer {
  DomainService base;
  int *clientSocks;
  int clientNum;
  int (* send)(struct DomainServer *self, UserMessage*, DomainHandle*);
  int (* receive)(struct DomainServer *self, UserMessage*, DomainHandle*);

} DomainServer;

int createClient(DomainClientOpts options, DomainClient** client);
int createServer(DomainServiceOpts options, DomainServer** server);


#endif