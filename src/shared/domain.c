/**
 *  WIP service for managing interactions between clients and servers while maintaining an open socket and abstracting
 *  serialization and deserialization.
 */


#include "domain_shared.h"
#include "domain_datagram.c"
#include "domain_stream.c"
#include "messaging/network.h"

static struct timeval createTimeout(const int timeoutMs) {
  const long timeoutS = timeoutMs / 1000;
  const long timeoutUs = timeoutMs % 1000 * 1000;
  const struct timeval timeout = {.tv_sec = timeoutS, .tv_usec = timeoutUs};
  return timeout;
}

/**
 * @param service
 * @param timeoutMs
 * @return
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


static void initializeGenericService(DomainServiceOpts options,
                                     DomainService *service) {
  service->sock = INACTIVE_SOCK;
  service->connectionType = options.connectionType;
  const int receiveTimeoutMs =
      options.receiveTimeoutMs > 0 ? options.receiveTimeoutMs : 0;
  service->receiveTimeout = createTimeout(receiveTimeoutMs);

  if (options.localPort > 0) {
    service->localAddr = getNetworkAddress(LOCALHOST, options.localPort);
  } else {
    service->localAddr = getNetworkAddress(LOCALHOST, 0);
  }
  service->incomingDeserializer = options.incomingDeserializer;
  service->outgoingSerializer = options.outgoingSerializer;
  service->start = startDatagramService;
  service->stop = stopDatagramService;
  service->changeTimeout = changeTimeout;
  service->destroy = destroyDatagramService;
}

int createServer(DomainServiceOpts options, DomainServer **server) {
  if (options.connectionType == STREAM) {
    printf("Stream server not currently supported!\n");
    return DOMAIN_FAILURE;
  }
  *server = calloc(1, sizeof(DomainServer));
  if (*server == NULL) {
    return DOMAIN_INIT_FAILURE;
  }
  if (options.connectionType == DATAGRAM) {
    (*server)->receive = datagramServerReceive;
    (*server)->send = datagramServerSend;
  } else {
    (*server)->clientNum = 0;
    (*server)->receive = streamServerReceive;
    (*server)->send = streamServerSend;
  }

  DomainService *serviceRef = (DomainService *) *server;
  initializeGenericService(options, serviceRef);

  return DOMAIN_SUCCESS;
}

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
  } else {
    (*client)->receive = streamClientReceive;
    (*client)->send = streamClientSend;
    (*client)->isConnected = false;
  }
  (*client)->remoteAddr = getNetworkAddress(options.remoteHost, options.remotePort);

  return DOMAIN_SUCCESS;
}
