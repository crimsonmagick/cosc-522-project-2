/**
 *  WIP service for managing interactions between clients and servers while maintaining an open socket and abstracting
 *  serialization and deserialization.
 */

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "domain_datagram.h"
#include "messaging/network.h"
#include "shared.h"

struct DatagramDomainService {
  int sock;
  struct sockaddr_in localAddr;
  MessageSerializer outgoingSerializer;
  MessageDeserializer incomingDeserializer;
};

/**
 * Deallocates handle and domain service
 * @param handle a non-null handle, with *handle and (*handle)->domainService non-null as well
 * @return DOMAIN_INIT_FAILURE value
 */
int failInit(DomainServiceHandle **handle) {
  free((*handle)->domainService);
  free(*handle);
  *handle = NULL;
  return DOMAIN_INIT_FAILURE;
}

/**
 * Handles memory allocation of service
 * @param handle  The return value, the abstracted handle
 * @return DOMAIN_INIT_FAILURE or DOMAIN_SUCCESS
 */
int allocateHandle(DomainServiceHandle **handle) {
  *handle = malloc(sizeof(DomainServiceHandle));
  if (*handle == NULL) {
    return DOMAIN_INIT_FAILURE;
  }
  (*handle)->domainService = calloc(1, sizeof(DatagramDomainService));
  if ((*handle)->domainService == NULL) {
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
int startDatagramService(const DomainServiceOpts options, DomainServiceHandle **handle) {
  if (allocateHandle(handle) == DOMAIN_INIT_FAILURE) {
    return DOMAIN_INIT_FAILURE;
  }
  DatagramDomainService *domainService = (*handle)->domainService;
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
    domainService->sock = getSocket(&domainService->localAddr, timeout, DATAGRAM);
  } else {
    domainService->sock = getSocket(NULL, timeout, DATAGRAM);
  }
  if (timeout != NULL) {
    free(timeout);
  }
  if (domainService->sock < 0) {
    return failInit(handle);
  }
  domainService->incomingDeserializer = options.incomingDeserializer;
  domainService->outgoingSerializer = options.outgoingSerializer;

  return DOMAIN_SUCCESS;
}

/**
 *
 * @param handle
 * @return
 */
int stopDatagramService(DomainServiceHandle **handle) {
  if (handle != NULL && *handle != NULL && (*handle)->domainService != NULL) {
    if ((*handle)->domainService->sock >= 0) {
      close((*handle)->domainService->sock);
    }
    free((*handle)->domainService);
  }
  if (handle != NULL && *handle != NULL) {
    free(*handle);
    *handle = NULL;
  }
  return DOMAIN_SUCCESS;
}

/**
 *
 * @param handle
 * @param message
 * @param hostAddr
 * @return
 */
int toDatagramDomainHost(DomainServiceHandle *handle, void *message, struct sockaddr_in *hostAddr) {
  char *buf = malloc(handle->domainService->outgoingSerializer.messageSize);
  const DatagramDomainService *service = handle->domainService;

  int status = DOMAIN_SUCCESS;

  if (service->outgoingSerializer.serializer(message, buf) == MESSAGE_SERIALIZER_FAILURE) {
    printf("Unable to serialize domain message\n");
    status = DOMAIN_FAILURE;
  } else if (sendDatagramMessage(service->sock, buf, service->outgoingSerializer.messageSize, hostAddr) == ERROR) {
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
int fromDatagramDomainHost(DomainServiceHandle *handle, void *message, struct sockaddr_in *hostAddr) {
  char *buf = malloc(handle->domainService->incomingDeserializer.messageSize);
  if (!buf) {
    printf("Failed to allocate message buffer\n");
    return DOMAIN_FAILURE;
  }
  const DatagramDomainService *service = handle->domainService;

  int status = DOMAIN_SUCCESS;

  if (receiveDatagramMessage(service->sock, buf, service->incomingDeserializer.messageSize, hostAddr)) {
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
int changeDatagramTimeout(DomainServiceHandle *handle, int timeoutMs) {
  struct timeval timeout;

  if (timeoutMs > 0) {
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = timeoutMs % 1000 * 1000;
  } else {
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
  }

  if (setsockopt(handle->domainService->sock, SOL_SOCKET, SO_RCVTIMEO,
                 &timeout, sizeof(timeout)) < 0) {
    return DOMAIN_FAILURE;
  }

  return DOMAIN_SUCCESS;
}
