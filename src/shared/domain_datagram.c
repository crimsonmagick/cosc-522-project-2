/**
 *  WIP service for managing interactions between clients and servers while maintaining an open socket and abstracting
 *  serialization and deserialization.
 */

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "domain_datagram.h"
#include "messaging/network.h"
#include "shared.h"

#define INACTIVE_SOCK (-1)

/**
 * Deallocates handle and domain service
 * @param service a non-null handle
 * @return DOMAIN_INIT_FAILURE value
 */
int failInit(DomainService **service) {
  free(*service);
  return DOMAIN_INIT_FAILURE;
}

/**
 * Handles memory allocation of service
 * @param service The return value, the service
 * @return DOMAIN_INIT_FAILURE or DOMAIN_SUCCESS
 */
int allocateHandle(DomainService **service) {
  *service = calloc(1, sizeof(DomainService));
  if (*service == NULL) {
    free(*service);
    return DOMAIN_INIT_FAILURE;
  }
  return DOMAIN_SUCCESS;
}

static int datagramClientSend(DomainClient *self, UserMessage *toSend) {
  return toDatagramDomainHost((DomainService *) self, toSend, &self->remoteAddr);
}

static int datagramClientReceive(DomainClient *self, UserMessage *toReceive) {
  struct sockaddr_in receiveAddr;
  int resp = fromDatagramDomainHost((DomainService *) self, toReceive, &receiveAddr);
  if (resp == DOMAIN_SUCCESS) {
    const int maxAttempts = 10;
    int attempt = 0;

    while (resp == DOMAIN_SUCCESS
           && receiveAddr.sin_addr.s_addr != self->remoteAddr.sin_addr.s_addr
           && receiveAddr.sin_port != self->remoteAddr.sin_port) {
      if (attempt > maxAttempts) {
        printf("Received more than %d messages from the wrong sending addr and port. Aborting...\n",
               maxAttempts);
        resp = DOMAIN_FAILURE;
        break;
      }
      printf("Warning... received message from unknown host. Discarding...\n");
      attempt += 1;
      resp = fromDatagramDomainHost((DomainService *) self, toReceive, &receiveAddr);
    }
  }
  return resp;
}

static int datagramServerSend(DomainServer *self, UserMessage *toSend,
                              DomainHandle *remoteTarget) {
  return toDatagramDomainHost((DomainService *) self, toSend, &remoteTarget->host);
}

static int datagramServerReceive(DomainServer *self, UserMessage *toReceive,
                                 DomainHandle *remote) {
  struct sockaddr_in receiveAddr;
  const int resp = fromDatagramDomainHost((DomainService *) self, toReceive, &receiveAddr);
  if (resp == DOMAIN_SUCCESS) {
    remote->userID = toReceive->userID;
    remote->host = receiveAddr;
  }
  return resp;
}

static struct timeval createTimeout(int timeoutMs) {
  const long timeoutS = timeoutMs / 1000;
  const long timeoutUs = timeoutMs % 1000 * 1000;
  const struct timeval timeout = {.tv_sec = timeoutS, .tv_usec = timeoutUs};
  return timeout;
}


static int startUdpService(DomainService *service) {
  const int sock = getSocket(&service->localAddr,
                       &service->sendTimeout,
                       service->connectionType);
  if (sock < 0) {
    return DOMAIN_FAILURE;
  }
  service->sock = sock;
  return DOMAIN_SUCCESS;
}

static int stopUdpService(DomainService *service) {
  if (close(service->sock) < 0) {
    return ERROR;
  }
  service->sock = INACTIVE_SOCK;
  return SUCCESS;
}

static void initializeGenericService(DomainServiceOpts options,
                              DomainService *service) {
  service->sock = INACTIVE_SOCK;
  const int receiveTimeoutMs =
      options.receiveTimeoutMs > 0 ? options.receiveTimeoutMs : 0;
  service->receiveTimeout = createTimeout(receiveTimeoutMs);

  if (options.localPort > 0) {
    service->localAddr = getNetworkAddress(LOCALHOST, options.localPort);
  } else {
    service->localAddr = getNetworkAddress(LOCALHOST, 0);
  }
  service->start = startUdpService;
  service->stop = stopUdpService;
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
  (*server)->receive = datagramServerReceive;
  (*server)->send = datagramServerSend;

  DomainService *serviceRef = ((DomainService *) (*server));
  initializeGenericService(options, serviceRef);

  return DOMAIN_SUCCESS;
}

int createClient(DomainClientOpts options, DomainClient **client) {
  if (options.baseOpts.connectionType == STREAM) {
    printf("Stream client not currently supported!\n");
    return DOMAIN_FAILURE;
  }
  *client= calloc(1, sizeof(DomainClient));
  if (*client == NULL) {
    return DOMAIN_FAILURE;
  }

  DomainService *serviceRef = (DomainService *) *client;
  initializeGenericService(options.baseOpts, serviceRef);

  (*client)->receive = datagramClientReceive;
  (*client)->send = datagramClientSend;
  (*client)->remoteAddr = getNetworkAddress(options.remoteHost, options.remotePort);

  return DOMAIN_SUCCESS;
}

/**
 * Constructor of a DomainService
 *
 * @param options
 * @param service
 * @return
 */
int startDatagramService(const DomainServiceOpts options,
                         DomainService **service) {
  if (allocateHandle(service) == DOMAIN_INIT_FAILURE) {
    return DOMAIN_INIT_FAILURE;
  }
  DomainService *domainService = *service;
  struct timeval *timeout = NULL;
  if (options.receiveTimeoutMs > 0) {
    timeout = malloc(sizeof(struct timeval));
    const long timeoutS = options.receiveTimeoutMs / 1000;
    const long timeoutUs = options.receiveTimeoutMs % 1000 * 1000;
    const struct timeval temp = {.tv_sec = timeoutS, .tv_usec = timeoutUs};
    *timeout = temp;
  }
  if (options.localPort > 0) {
    domainService->localAddr =
        getNetworkAddress(LOCALHOST, options.localPort);
    domainService->sock =
        getSocket(&domainService->localAddr, timeout, DATAGRAM);
  } else {
    domainService->sock = getSocket(NULL, timeout, DATAGRAM);
  }
  if (timeout != NULL) {
    free(timeout);
  }
  if (domainService->sock < 0) {
    return failInit(service);
  }
  domainService->incomingDeserializer = options.incomingDeserializer;
  domainService->outgoingSerializer = options.outgoingSerializer;

  return DOMAIN_SUCCESS;
}

/**
 *
 * @param service
 * @return
 */
int stopDatagramService(DomainService **service) {
  if (*service != NULL) {
    if ((*service)->sock >= 0) {
      close((*service)->sock);
    }
    free(*service);
  }
  return DOMAIN_SUCCESS;
}

/**
 *
 * @param service
 * @param message
 * @param hostAddr
 * @return
 */
int toDatagramDomainHost(DomainService *service,
                         void *message,
                         struct sockaddr_in *hostAddr) {
  char *buf = malloc(service->outgoingSerializer.messageSize);

  int status = DOMAIN_SUCCESS;

  if (service->outgoingSerializer.serializer(message, buf) ==
      MESSAGE_SERIALIZER_FAILURE) {
    printf("Unable to serialize domain message\n");
    status = DOMAIN_FAILURE;
  } else if (sendDatagramMessage(service->sock,
                                 buf,
                                 service->outgoingSerializer.messageSize,
                                 hostAddr) == ERROR) {
    printf("Unable to send message to domain\n");
    status = DOMAIN_FAILURE;
  }

  free(buf);
  return status;
}

/**
 *
 * @param handle
 * @param message
 * @param hostAddr
 * @return
 */
int fromDatagramDomainHost(DomainService *service,
                           void *message,
                           struct sockaddr_in *hostAddr) {
  char *buf = malloc(service->incomingDeserializer.messageSize);
  if (!buf) {
    printf("Failed to allocate message buffer\n");
    return DOMAIN_FAILURE;
  }

  int status = DOMAIN_SUCCESS;

  if (receiveDatagramMessage(service->sock,
                             buf,
                             service->incomingDeserializer.messageSize,
                             hostAddr)) {
    printf("Unable to receive message from domain\n");
    status = DOMAIN_FAILURE;
  } else if (service->incomingDeserializer.deserializer(buf, message) ==
             MESSAGE_DESERIALIZER_FAILURE) {
    printf("Unable to deserialize domain message\n");
    status = DOMAIN_FAILURE;
  }
  free(buf);
  return status;
}

/**
 * @param service
 * @param timeoutMs
 * @return
 */
int changeTimeout(DomainService *service, int timeoutMs) {
  const struct timeval timeout = createTimeout(timeoutMs);
  if (setsockopt(service->sock, SOL_SOCKET, SO_RCVTIMEO,
                 &timeout, sizeof(timeout)) < 0) {
    return DOMAIN_FAILURE;
  }

  return DOMAIN_SUCCESS;
}
