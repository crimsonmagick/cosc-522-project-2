#include "domain_stream_shared.h"

static int streamClientFromHost(DomainClient *client, void *message) {
  char *buf = malloc(client->base.incomingDeserializer.messageSize);
  if (!buf) {
    printf("Failed to allocate message buffer\n");
    return DOMAIN_FAILURE;
  }

  int status = DOMAIN_SUCCESS;

  int recvStatus = receiveTcpMessage(client->base.sock, buf, client->base.incomingDeserializer.messageSize);
  if (recvStatus == ERROR) {
    printf("Unable to receive message from domain\n");
    status = DOMAIN_FAILURE;
  } else if (recvStatus == TERMINATED) {
    client->isConnected = false;
    status = TERMINATED;
  } else if (client->base.incomingDeserializer.deserializer(buf, message) ==
             MESSAGE_DESERIALIZER_FAILURE) {
    printf("Unable to deserialize domain message\n");
    status = DOMAIN_FAILURE;
  }
  free(buf);
  return status;
}

static int streamClientSend(DomainClient *self, UserMessage *toSend) {
  if (!self->isConnected && tcpConnect(self->base.sock, &self->remoteAddr) == ERROR) {
    printf("Stream Client: Error connecting to server, aborting...\n");
    return DOMAIN_FAILURE;
  }
  self->isConnected = true;
  return toStreamDomainHost((DomainService *) self, toSend, self->base.sock);
}

static int streamClientReceive(DomainClient *self, UserMessage *toReceive) {
  if (!self->isConnected) {
    printf("Stream Client: Should always be connected before receiving, aborting...\n");
  }
  return streamClientFromHost(self, toReceive);
}


static int startStreamClient(DomainService *service) {
  const int sock = getSocket(&service->localAddr,
                             &service->receiveTimeout,
                             service->connectionType);
  if (sock < 0) {
    return DOMAIN_FAILURE;
  }
  service->sock = sock;
  return DOMAIN_SUCCESS;
}

static int stopStreamClient(DomainService *service) {
  if (stopStreamService(service) != DOMAIN_FAILURE) {
    ((DomainClient *) service)->isConnected = false;
    return DOMAIN_SUCCESS;
  }
  return DOMAIN_FAILURE;
}