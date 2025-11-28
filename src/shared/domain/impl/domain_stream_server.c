#include "domain_shared.h"

static int streamServerSend(DomainServer *self, UserMessage *toSend,
                            ClientHandle *remoteTarget) {
  return toStreamDomainHost((DomainService *) self, toSend, remoteTarget->clientSock);
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
    status = TERMINATED;
  } else if (server->base.incomingDeserializer.deserializer(buf, message) ==
             MESSAGE_DESERIALIZER_FAILURE) {
    printf("Unable to deserialize domain message\n");
    status = DOMAIN_FAILURE;
  }
  free(buf);
  return status;
}

// returns max FD
static int setUpDescriptors(DomainServer *self, fd_set *allSocks) {
  FD_ZERO(allSocks);
  FD_SET(self->base.sock, allSocks);
  int maxFd = self->base.sock;
  List *clients = self->clients;
  for (int i = 0; i < clients->length; i++) {
    ClientHandle *client;
    clients->get(clients, i, (void **) &client);
    if (client->clientSock > maxFd) {
      maxFd = client->clientSock;
    }
    FD_SET(client->clientSock, allSocks);
  }
  return maxFd;
}

static bool isTimeoutEnabled(struct timeval * timeout) {
  return timeout != NULL && (timeout->tv_sec > 0 || timeout->tv_usec > 0);
}

static int streamServerReceive(DomainServer *self, UserMessage *toReceiveOut,
                               ClientHandle *clientHandleOut) {
  while (true) {
    fd_set allSocks;
    const int maxFd = setUpDescriptors(self, &allSocks);
    struct timeval *timeout = isTimeoutEnabled(&self->base.receiveTimeout) ? &self->base.receiveTimeout : NULL;
    const int rv = select(maxFd + 1, &allSocks, NULL, NULL, timeout);
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
      ClientHandle *client = malloc(sizeof(ClientHandle));
      client->userID = NO_USER;
      client->clientSock = clientSock;
      client->clientAddr = clientAddr;
      self->clients->append(self->clients, client);
    } else {
      for (int i = 0; i < self->clients->length; i++) {
        ClientHandle *clientHandle;
        self->clients->get(self->clients, i, (void **) &clientHandle);
        if (FD_ISSET(clientHandle->clientSock, &allSocks)) {
          const int resp = streamServerFromHost(self, toReceiveOut, clientHandle->clientSock);
          if (resp == DOMAIN_SUCCESS) {
            clientHandle->userID = toReceiveOut->userID;
            clientHandleOut->userID = clientHandle->userID;
            clientHandleOut->clientSock = clientHandle->clientSock;
            clientHandleOut->clientAddr = clientHandle->clientAddr;
          } else if (resp == TERMINATED) {
            if (self->clients->remove(self->clients, i, (void **) &clientHandleOut) == ERROR) {
              printf("Stream Server: remove failed\n");
            }
            close(clientHandleOut->clientSock);
            free(clientHandleOut);
          }
          return resp;
        }
      }
    }
  }
}

static int startStreamServer(DomainService *service) {
  const int sock = getSocket(&service->localAddr,
                             &service->receiveTimeout,
                             service->connectionType);
  if (sock < 0) {
    return DOMAIN_FAILURE;
  }
  service->sock = sock;
  if (tcpListen(sock) == ERROR) {
    printf("Stream Serve: Unable to listen for incoming messages\n");
    close(sock);
    service->sock = INACTIVE_SOCK;
    return DOMAIN_FAILURE;
  }
  DomainServer *impl = (DomainServer *) service;
  createList(&impl->clients);
  return DOMAIN_SUCCESS;
}
