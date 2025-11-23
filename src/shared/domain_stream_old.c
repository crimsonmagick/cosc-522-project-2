/**
 *  WIP service for managing interactions between clients and servers while maintaining an open socket and abstracting
 *  serialization and deserialization.
 */

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "domain_stream_old.h"
#include "messaging/network.h"
#include "shared.h"

struct StreamDomainService {
  int sock;
  int clientSock;
  int isServer;
  struct sockaddr_in localAddr;
  struct sockaddr_in remoteAddr;
  MessageSerializer outgoingSerializer;
  MessageDeserializer incomingDeserializer;
};

/**
 * Deallocates handle and domain service
 * @param handle a non-null handle, with *handle and (*handle)->domainService non-null as well
 * @return DOMAIN_INIT_FAILURE value
 */
static int failInit(StreamDomainServiceHandle **handle) {
  free((*handle)->streamDomainService);
  free(*handle);
  *handle = NULL;
  return DOMAIN_INIT_FAILURE;
}

/**
 * Handles memory allocation of service
 * @param handle  The return value, the abstracted handle
 * @return DOMAIN_INIT_FAILURE or DOMAIN_SUCCESS
 */
static int streamAllocateHandle(StreamDomainServiceHandle **handle) {
  *handle = malloc(sizeof(StreamDomainServiceHandle));
  if (*handle == NULL) {
    return DOMAIN_INIT_FAILURE;
  }
  (*handle)->streamDomainService = calloc(1, sizeof(StreamDomainService));
  if ((*handle)->streamDomainService == NULL) {
    free(*handle);
    *handle = NULL;
    return DOMAIN_INIT_FAILURE;
  }
  return DOMAIN_SUCCESS;
}


/**
 * Constructor of a DomainService
 *
 * @param options
 * @param handle
 * @return
 */
int startStreamService(const StreamDomainServiceOpts options, StreamDomainServiceHandle **handle) {
  if (streamAllocateHandle(handle) == DOMAIN_INIT_FAILURE) {
    return DOMAIN_INIT_FAILURE;
  }
  StreamDomainService *domainService = (*handle)->streamDomainService;
  struct timeval *timeout = NULL;
  if (options.timeoutMs > 0) {
    timeout = malloc(sizeof(struct timeval));
    const long timeoutS = options.timeoutMs / 1000;
    const long timeoutUs = options.timeoutMs % 1000 * 1000;
    const struct timeval temp = {.tv_sec = timeoutS, .tv_usec = timeoutUs};
    *timeout = temp;
  }
  if (options.localPort != NULL) {
    domainService->localAddr = getNetworkAddress(LOCALHOST, atoi(options.localPort));
    domainService->sock = getSocket(&domainService->localAddr, timeout, STREAM);
  } else {
    domainService->sock = getSocket(NULL, timeout, STREAM);
  }
  if (timeout != NULL) {
    free(timeout);
  }
  if (domainService->sock < 0) {
    return failInit(handle);
  }
  domainService->incomingDeserializer = options.incomingDeserializer;
  domainService->outgoingSerializer = options.outgoingSerializer;

  domainService->isServer = options.isServer;
  if (options.isServer) {
    if (tcpListen(domainService->sock) == ERROR) {
      printf("Unable to listen for incoming messages\n");
      return DOMAIN_FAILURE;
    }
    struct sockaddr_in clientAddress;
    int clientSock;
    if (tcpAccept(domainService->sock, &clientAddress, &clientSock) == ERROR) {
      printf("Unable to accept client connection...\n");
      return DOMAIN_FAILURE;
    }
    domainService->remoteAddr = clientAddress;
    domainService->clientSock = clientSock;
  } else {
    if (tcpConnect(domainService->sock, &options.remoteHost) == ERROR) {
      return DOMAIN_FAILURE;
    }
  }
  return DOMAIN_SUCCESS;
}

/**
 *
 * @param handle
 * @return
 */
int stopStreamService(StreamDomainServiceHandle **handle) {
  if (handle != NULL && *handle != NULL && (*handle)->streamDomainService != NULL) {
    if ((*handle)->streamDomainService->sock >= 0) {
      close((*handle)->streamDomainService->sock);
    }
    if ((*handle)->streamDomainService->clientSock>= 0) {
      close((*handle)->streamDomainService->clientSock);
    }

    free((*handle)->streamDomainService);
  }
  if (handle != NULL && *handle != NULL) {
    free(*handle);
    *handle = NULL;
  }
  return DOMAIN_SUCCESS;
}

int toStreamDomainHost(StreamDomainServiceHandle *handle, void *message) {
  char *buf = malloc(handle->streamDomainService->outgoingSerializer.messageSize);
  const StreamDomainService *service = handle->streamDomainService;

  int status = DOMAIN_SUCCESS;
  int sock = handle->streamDomainService->isServer
               ? handle->streamDomainService->clientSock
               : handle->streamDomainService->sock;

  if (service->outgoingSerializer.serializer(message, buf) == MESSAGE_SERIALIZER_FAILURE) {
    printf("Unable to serialize domain message\n");
    status = DOMAIN_FAILURE;
  } else if (sendTcpMessage(sock, buf, service->outgoingSerializer.messageSize) == ERROR) {
    printf("Unable to send message to domain\n");
    status = DOMAIN_FAILURE;
  }
  free(buf);
  return status;
}

int fromStreamDomainHost(StreamDomainServiceHandle *handle, void *message) {
  char *buf = malloc(handle->streamDomainService->incomingDeserializer.messageSize);
  if (!buf) {
    printf("Failed to allocate message buffer\n");
    return DOMAIN_FAILURE;
  }
  const StreamDomainService *service = handle->streamDomainService;

  int status = DOMAIN_SUCCESS;

  int sock;
  if (handle->streamDomainService->isServer) {
    sock = handle->streamDomainService->clientSock;
  } else {
    sock = handle->streamDomainService->sock;
  }

  if (receiveTcpMessage(sock, buf, service->incomingDeserializer.messageSize)) {
    printf("Unable to receive message from domain\n");
    status = DOMAIN_FAILURE;
  } else if (service->incomingDeserializer.deserializer(buf, message) == MESSAGE_DESERIALIZER_FAILURE) {
    printf("Unable to deserialize domain message\n");
    status = DOMAIN_FAILURE;
  }
  free(buf);
  return status;
}

/**
 * FIXME should probably go into UDP?
 * @param handle
 * @param timeoutMs
 * @return
 */
int changeStreamTimeout(StreamDomainServiceHandle *handle, int timeoutMs) {
  struct timeval timeout;

  if (timeoutMs > 0) {
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = timeoutMs % 1000 * 1000;
  } else {
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
  }

  if (setsockopt(handle->streamDomainService->sock, SOL_SOCKET, SO_RCVTIMEO,
                 &timeout, sizeof(timeout)) < 0) {
    return DOMAIN_FAILURE;
  }

  return DOMAIN_SUCCESS;
}
