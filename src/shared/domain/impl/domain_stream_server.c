/**
* Implementation of core TCP Server
*/

#include "domain_stream_shared.h"

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

/**
 * Initializes fd_set
 *
 * @param self
 * @param allSocks set to initialize
 * @return The max file descriptor
 */
static int initDescriptors(DomainServer *self, fd_set *allSocks) {
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

/**
 * Tests timeval struct to determine if it's non-zero.
 *
 * @param timeout to evaluate
 * @return true, false
 */
static bool isTimeoutEnabled(struct timeval * timeout) {
  return timeout != NULL && (timeout->tv_sec > 0 || timeout->tv_usec > 0);
}

/**
 * Stream/TCP implementation of DomainServer#receive
 *
 * Utilizes select() to multiplex between accepting new connections (via the server's socket) and multiple concurrently
 * active client sockets.
 *
 * @param self self-reference
 * @param toReceiveOut inbound message
 * @param clientCallbackOut client details associated with the message
 * @return
 */
static int streamServerReceive(DomainServer *self, UserMessage *toReceiveOut,
                               ClientHandle *clientCallbackOut) {
  while (true) {
    fd_set allSocks;
    const int maxFd = initDescriptors(self, &allSocks);
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
    // do we have a new connection we need to accept()?
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
      // if there's no pending connections, at least one of the persistent clients sockets must have data available
      for (int i = 0; i < self->clients->length; i++) {
        ClientHandle *clientHandle;
        self->clients->get(self->clients, i, (void **) &clientHandle);
        if (FD_ISSET(clientHandle->clientSock, &allSocks)) {
          // we found a socket needing servicing!
          const int resp = streamServerFromHost(self, toReceiveOut, clientHandle->clientSock);
          if (resp == DOMAIN_SUCCESS) {
            // received client message
            clientHandle->userID = toReceiveOut->userID;
            clientCallbackOut->userID = clientHandle->userID;
            clientCallbackOut->clientSock = clientHandle->clientSock;
            clientCallbackOut->clientAddr = clientHandle->clientAddr;
          } else if (resp == TERMINATED) {
            // client terminated connection - remove from list of clients and inform caller in case they're interested
            if (self->clients->remove(self->clients, i, (void **) &clientCallbackOut) == ERROR) {
              printf("[ERROR] Stream Server: remove failed\n");
            }
            close(clientCallbackOut->clientSock);
            free(clientCallbackOut);
          }
          return resp;
        }
      }
    }
  }
}

/**
 * @see DomainService#start
 */
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
