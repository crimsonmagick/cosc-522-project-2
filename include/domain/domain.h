/**
 *  Services for managing interactions between clients and servers while maintaining an open socket and abstracting
 *  serialization and deserialization.
 */

#ifndef COSC522_LODI_DOMAIN_H
#define COSC522_LODI_DOMAIN_H

#include <netinet/in.h>
#include <stdbool.h>

#include "collections/list.h"
#include "domain/domain.h"
#include "util/network.h"


#define DOMAIN_SUCCESS 0
#define DOMAIN_FAILURE 1

#define MESSAGE_SERIALIZER_SUCCESS 0
#define MESSAGE_SERIALIZER_FAILURE 1
#define MESSAGE_DESERIALIZER_SUCCESS 0
#define MESSAGE_DESERIALIZER_FAILURE 1

#define DEFAULT_TIMEOUT_MS 500

/**
 * Responsible for serializing user struct into network-portable bytes
 */
typedef struct MessageSerializer {
  size_t messageSize;


  /**
  * @param input Input data, length specified by messageSize
  * @param output Output bytes, size is messageSize
  *
  * @return MESSAGE_SERIALIZER_SUCCESS or MESSAGE_SERIALIZER_FAILURE
  */
  int (*serializer)(void *input, char *output);
} MessageSerializer;

typedef struct MessageDeserializer {
  size_t messageSize;

  /**
  * @param input Input data, length specified by messageSize
  * @param output Output bytes, size is messageSize
  *
  * @return MESSAGE_DESERIALIZER_SUCCESS or MESSAGE_DESERIALIZER_FAILURE
  */
  int (*deserializer)(char *input, void *output);
} MessageDeserializer;

typedef struct {
  unsigned int messageType; /* placeholder for implementations */
  unsigned int userID; /* user identifier, common to all messages*/
  // struct may have arbitrary fields contiguously in memory after the userID
} UserMessage;

/**
 * Options for Service creation.
 */
typedef struct DomainServiceOpts {
  int localPort; // optional
  char *localHost; // required for server
  int receiveTimeoutMs; // optional
  enum ConnectionType connectionType; // required
  MessageSerializer outgoingSerializer; // required
  MessageDeserializer incomingDeserializer; // required
} DomainServiceOpts;

/**
 * Additional configuration options
 */
typedef struct DomainClientOpts {
  DomainServiceOpts baseOpts; // base struct
  int remotePort; // required
  char *remoteHost; //required
} DomainClientOpts;

/**
 * Interface to accessing one of the 3 project domains: PKE, TFA, or Lodi
 */
typedef struct DomainService {
  int sock; // only used if ConnectionType is Stream
  enum ConnectionType connectionType;
  struct sockaddr_in localAddr;
  struct timeval receiveTimeout;

  MessageSerializer outgoingSerializer;
  MessageDeserializer incomingDeserializer;

  /**
   * Starts the service, putting it in a state where it can start processing messages
   *
   * @param self
   */
  int (*start)(struct DomainService * self);

  /**
   * Stops the service, putting it in a state where it can no longer processing messages. Frees network resources.
   *
   * @param self
   */
  int (*stop)(struct DomainService *self);

  /**
   * Destroys and deallocates the service, freeing network resources if necessary.
   *
   * @param self
   */
  int (*destroy)(struct DomainService ** self);

  int (*changeTimeout)(struct DomainService *, int timeoutMs);
} DomainService;

/**
 * Interface for interacting with a Datagram or Stream client.
 */
typedef struct DomainClient {
  DomainService base;
  struct sockaddr_in remoteAddr; // remote server the client is dedicated to
  bool isConnected; // is the client currently connected? Only used for Stream clients

  /**
  * Sends a message to the client represented by a ClientHandle.
  *
  * @param self specific client instance
  * @param message Input message to be sent to the server
  * @return DOMAIN_SUCCESS or DOMAIN_FAILURE
  */
  int (*send)(struct DomainClient *self, UserMessage *message);

  /**
  * Receives a message from the client's server
  *
  * @param self specific server instance
  * @param receivedOut Caller-allocated space for the message received by the server
  * @return DOMAIN_SUCCESS when a message has been successfully received and deserialized
  *         DOMAIN_FAILURE when message reception or deserialization has failed
  *         If server's ConnectionType is STREAM, TERMINATED may be returned when a persistent connection with the
  *         server has been closed by the server.
  */
  int (*receive)(struct DomainClient *self, UserMessage *receivedOut);
} DomainClient;

/**
 * Encapsulates specific client details; when available, the handle is userId aware.
 */
typedef struct ClientHandle {
  unsigned int userID; // OPTIONAL - will be set to NO_USER if the userID is unknown
  struct sockaddr_in clientAddr; // client's network address details
  int clientSock; // OPTIONAL - only used if ConnectionType is STREAM
} ClientHandle;

/**
 * Interface for interacting with a Datagram or Stream server.
 */
typedef struct DomainServer {
  DomainService base;
  List *clients;

  /**
  * Sends a message to the client represented by a ClientHandle.
  *
  * @param self specific server instance
  * @param message Input message to be sent to the client
  * @param clientHandle Client details
  * @return DOMAIN_SUCCESS or DOMAIN_FAILURE
  */
  int (*send)(struct DomainServer *self, UserMessage *message, ClientHandle *clientHandle);

  /**
  * Receives inbound messages from a client.
  *
  * @param self specific server instance
  * @param receivedOut Caller-allocated space for the message received by the server
  * @param clientCallbackOut Caller-allocated space for a ClientHandle callback.
  * @return DOMAIN_SUCCESS when a message has been successfully received and deserialized
  *         DOMAIN_FAILURE when message reception or deserialization has failed
  *         If server's ConnectionType is STREAM, TERMINATED may be returned when persistent connection has been closed
  *         by the client.
  */
  int (*receive)(struct DomainServer *self, UserMessage *receivedOut, ClientHandle *clientCallbackOut);
} DomainServer;

/**
 * Creates a new DomainClient - the function is responsible for allocating the space for the new client.
 *
 * @param options Configuration options for the domain
 * @param client Output - points to the new client
 * @return DOMAIN_SUCCESS or DOMAIN_FAILURE
 */
int createClient(DomainClientOpts options, DomainClient **client);

/**
 * Creates a new DomainServer - the function is responsible for allocating the space for the new client.
 *
 * @param options Configuration options for the domain
 * @param server Output - points to the newly created server
 * @return DOMAIN_SUCCESS or DOMAIN_FAILURE
 */
int createServer(DomainServiceOpts options, DomainServer **server);

#endif
