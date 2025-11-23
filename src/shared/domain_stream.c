#include "domain_shared.h"

static int stopStreamService(DomainService *service) {
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
static int destroyStreamService(DomainService **service) {
  if (*service != NULL) {
    if (stopStreamService(*service) == DOMAIN_FAILURE) {
      return DOMAIN_FAILURE;
    }
    free(*service);
  }
  return DOMAIN_SUCCESS;
}

static int toStreamDomainHost(DomainService *service, void *message, const int sock) {
  char *buf = malloc(service->outgoingSerializer.messageSize);

  int status = DOMAIN_SUCCESS;

  if (service->outgoingSerializer.serializer(message, buf) ==
      MESSAGE_SERIALIZER_FAILURE) {
    printf("Unable to serialize domain message\n");
    status = DOMAIN_FAILURE;
  } else if (sendTcpMessage(sock, buf, service->outgoingSerializer.messageSize) == ERROR) {
    printf("Unable to send message to domain\n");
    status = DOMAIN_FAILURE;
  }

  free(buf);
  return status;
}

static int streamServerFromHost(DomainServer *server, void *message, int sock) {
  char *buf = malloc(server->base.incomingDeserializer.messageSize);
  if (!buf) {
    printf("Failed to allocate message buffer\n");
    return DOMAIN_FAILURE;
  }

  int status = DOMAIN_SUCCESS;

  int recvStatus = receiveTcpMessage(sock, buf, server->base.incomingDeserializer.messageSize);
  if (recvStatus == ERROR) {
    printf("Unable to receive message from domain\n");
    status = DOMAIN_FAILURE;
  } else if (recvStatus == TERMINATED) {
    for (int i = 0; i < server->clientNum; i++) {
      if (server->clientSocks[i] == sock) {
        close(server->clientSocks[i]);
        server->clientSocks[i] = INACTIVE_SOCK;
        server->clientNum--;
      }
    }
    status = TERMINATED;
  } else if (server->base.incomingDeserializer.deserializer(buf, message) ==
             MESSAGE_DESERIALIZER_FAILURE) {
    printf("Unable to deserialize domain message\n");
    status = DOMAIN_FAILURE;
  }
  free(buf);
  return status;
}

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
  streamClientFromHost(self, toReceive);
  return DOMAIN_SUCCESS;
}

static int streamServerSend(DomainServer *self, UserMessage *toSend,
                            DomainHandle *remoteTarget) {
  return toStreamDomainHost((DomainService *) self, toSend, remoteTarget->clientSock);
}

static int streamServerReceive(DomainServer *self, UserMessage *toReceive,
                               DomainHandle *remote) {
  while (true) {
    fd_set allSocks;
    FD_ZERO(&allSocks);
    FD_SET(self->base.sock, &allSocks);
    int maxFd = self->base.sock;
    for (int i = 0; i < self->clientNum; i++) {
      if (self->clientSocks[i] > maxFd) {
        maxFd = self->clientSocks[i];
      }
      FD_SET(self->clientSocks[i], &allSocks);
    }
    int rv = select(maxFd +1, &allSocks, NULL, NULL, &self->base.receiveTimeout);
    if (rv == -1) {
      printf("Stream Server: select() failed\n");
      return DOMAIN_FAILURE;
    }
    if (rv == 0) {
      printf("Stream Server: select timeout\n");
      return DOMAIN_FAILURE;
    }
    if (FD_ISSET(self->base.sock, &allSocks)) {
      struct sockaddr_in clientAddr;
      int clientSock;
      if (tcpAccept(self->base.sock, &clientAddr, &clientSock) == ERROR) {
        printf("Stream Server: accept failed\n");
        return DOMAIN_FAILURE;
      }
      self->clientSocks[self->clientNum] = clientSock;
      self->clientNum++;
    } else {
        for (int i = 0; i < self->clientNum; i++) {
          if (FD_ISSET(self->clientSocks[i], &allSocks)) {
            const int resp = streamServerFromHost(self, toReceive, self->clientSocks[i]);
            if (resp == DOMAIN_SUCCESS) {
              remote->userID = toReceive->userID;
              remote->clientSock = self->clientSocks[i];
            }
            return resp;
          }
        }
      }
    }
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

static int startStreamServer(DomainService *service) {
  const int sock = getSocket(&service->localAddr,
                             &service->receiveTimeout,
                             service->connectionType);
  if (sock < 0) {
    return DOMAIN_FAILURE;
  }
  service->sock = sock;
  return DOMAIN_SUCCESS;
  if (tcpListen(sock) == ERROR) {
    printf("Stream Serve: Unable to listen for incoming messages\n");
    close(sock);
    service->sock = INACTIVE_SOCK;
    return DOMAIN_FAILURE;
  }
  return DOMAIN_FAILURE;
}
