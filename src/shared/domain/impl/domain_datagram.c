#include "domain_shared.h"

static int stopDatagramService(DomainService *service) {
  if (service->sock >= 0 && close(service->sock) < 0) {
    return DOMAIN_FAILURE;
  }
  service->sock = INACTIVE_SOCK;
  return DOMAIN_SUCCESS;
}

/**
 *
 * @param service
 * @return
 */
static int destroyDatagramService(DomainService **service) {
  if (*service != NULL) {
    if (stopDatagramService(*service) == DOMAIN_FAILURE) {
      return DOMAIN_FAILURE;
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
static int toDatagramDomainHost(DomainService *service,
                                void *message,
                                struct sockaddr_in *hostAddr) {
  char *buf = malloc(service->outgoingSerializer.messageSize);

  int status = DOMAIN_SUCCESS;

  if (service->outgoingSerializer.serializer(message, buf) ==
      MESSAGE_SERIALIZER_FAILURE) {
    printf("Unable to serialize domain message\n");
    status = DOMAIN_FAILURE;
  } else if (sendUdpMessage(service->sock,
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
 * @param service
 * @param message
 * @param hostAddr
 * @return
 */
static int fromDatagramDomainHost(DomainService *service,
                           void *message,
                           struct sockaddr_in *hostAddr) {
  char *buf = malloc(service->incomingDeserializer.messageSize);
  if (!buf) {
    printf("Failed to allocate message buffer\n");
    return DOMAIN_FAILURE;
  }

  int status = DOMAIN_SUCCESS;

  if (receiveUdpMessage(service->sock,
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
                              ClientHandle *remoteTarget) {
  return toDatagramDomainHost((DomainService *) self, toSend, &remoteTarget->clientAddr);
}

static int datagramServerReceive(DomainServer *self, UserMessage *toReceive,
                                 ClientHandle *remote) {
  struct sockaddr_in receiveAddr;
  const int resp = fromDatagramDomainHost((DomainService *) self, toReceive, &receiveAddr);
  if (resp == DOMAIN_SUCCESS) {
    remote->userID = toReceive->userID;
    remote->clientAddr = receiveAddr;
  }
  return resp;
}

static int startDatagramService(DomainService *service) {
  const int sock = getSocket(&service->localAddr,
                             &service->receiveTimeout,
                             service->connectionType);
  if (sock < 0) {
    return DOMAIN_FAILURE;
  }
  service->sock = sock;
  return DOMAIN_SUCCESS;
}
