/**
 *  Services for managing interactions between clients and servers while maintaining an open socket and abstracting
 *  serialization and deserialization.
 */

#include "domain_datagram.c"
#include "domain_stream_client.c"
#include "domain_stream_server.c"

static struct timeval createTimeout(const int timeoutMs) {
  const long timeoutS = timeoutMs / 1000;
  const long timeoutUs = timeoutMs % 1000 * 1000;
  const struct timeval timeout = {.tv_sec = timeoutS, .tv_usec = timeoutUs};
  return timeout;
}

/**
 * Changes receive timeout for a service.
 *
 * @param service to-modify
 * @param timeoutMs receive timeout in ms
 * @return DOMAIN_SUCCESS or DOMAIN_FAILURE
 */
static int changeTimeout(DomainService *service, const int timeoutMs) {
  const struct timeval timeout = createTimeout(timeoutMs);
  service->receiveTimeout = timeout;
  if (setsockopt(service->sock, SOL_SOCKET, SO_RCVTIMEO,
                 &timeout, sizeof(timeout)) < 0) {
    return DOMAIN_FAILURE;
  }
  return DOMAIN_SUCCESS;
}

/**
 * Initializer function common to both clients and servers
 *
 * @param options configuration options
 * @param service to-initialize
 */
static void initializeGenericService(DomainServiceOpts options,
                                     DomainService *service) {
  service->sock = INACTIVE_SOCK;
  service->connectionType = options.connectionType;
  const int receiveTimeoutMs =
      options.receiveTimeoutMs > 0 ? options.receiveTimeoutMs : 0;
  service->receiveTimeout = createTimeout(receiveTimeoutMs);
  const char * localAddress = options.localHost == NULL ? LOCALHOST : options.localHost;
  const int localPort = options.localPort > 0 ? options.localPort : 0;

  service->localAddr = getNetworkAddress(localAddress, localPort);
  service->incomingDeserializer = options.incomingDeserializer;
  service->outgoingSerializer = options.outgoingSerializer;
  service->changeTimeout = changeTimeout;
  service->destroy = destroyDatagramService;
}

/**
 * Allocates and configures a new DomainServer.
 *
 * @param options configuration options
 * @param server newly allocated and configured server
 * @return DOMAIN_SUCCESS, DOMAIN_FAILURE
 */
int createServer(const DomainServiceOpts options, DomainServer **server) {
  *server = calloc(1, sizeof(DomainServer));
  if (*server == NULL) {
    return DOMAIN_FAILURE;
  }
  if (options.connectionType == DATAGRAM) {
    (*server)->base.start = startDatagramServer;
    (*server)->base.stop = stopDatagramService;
    (*server)->receive = datagramServerReceive;
    (*server)->send = datagramServerSend;
  } else {
    (*server)->base.start = startStreamServer;
    (*server)->base.stop = stopStreamService;
    (*server)->receive = streamServerReceive;
    (*server)->send = streamServerSend;
  }
  (*server)->clients = NULL;

  DomainService *serviceRef = (DomainService *) *server;
  initializeGenericService(options, serviceRef);

  return DOMAIN_SUCCESS;
}

/**
 * Allocates and configures a new DomainClient.
 *
 * @param options configuration options
 * @param client newly allocated and configured client
 * @return DOMAIN_SUCCESS, DOMAIN_FAILURE
 */
int createClient(DomainClientOpts options, DomainClient **client) {
  *client = calloc(1, sizeof(DomainClient));
  if (*client == NULL) {
    return DOMAIN_FAILURE;
  }

  DomainService *serviceRef = (DomainService *) *client;
  initializeGenericService(options.baseOpts, serviceRef);

  if (options.baseOpts.connectionType == DATAGRAM) {
    (*client)->receive = datagramClientReceive;
    (*client)->send = datagramClientSend;
    (*client)->base.start = startDatagramClient;
    (*client)->base.stop = stopDatagramService;
  } else {
    (*client)->receive = streamClientReceive;
    (*client)->send = streamClientSend;
    (*client)->isConnected = false;
    (*client)->base.start = startStreamClient;
    (*client)->base.stop = stopStreamClient;
  }
  (*client)->remoteAddr = getNetworkAddress(options.remoteHost, options.remotePort);
  return DOMAIN_SUCCESS;
}
