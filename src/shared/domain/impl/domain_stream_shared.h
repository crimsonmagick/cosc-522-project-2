/**
* Shared functions between TCP Server and Client
*/

#ifndef COSC522_LODI_DOMAIN_STREAM_SHARED_H
#define COSC522_LODI_DOMAIN_STREAM_SHARED_H
#include "domain_shared.h"

/**
 * Sends a domain message to a host with the specified socket
 *
 * @param service self-reference
 * @param message structured message to send
 * @param sock socket to send the message on
 * @return DOMAIN_SUCCESS, DOMAIN_FAILURE
 */
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

/**
 *  @see DomainService#stop
 */
static int stopStreamService(DomainService *service) {
  if (service->sock >= 0 && close(service->sock) < 0) {
    return DOMAIN_FAILURE;
  }
  service->sock = INACTIVE_SOCK;
  return DOMAIN_SUCCESS;
}

/**
 *  @see DomainService#destroy
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

#endif
