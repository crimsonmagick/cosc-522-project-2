#ifndef COSC522_LODI_DOMAIN_SHARED_H
#define COSC522_LODI_DOMAIN_SHARED_H

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "domain.h"
#include "shared.h"

#define INACTIVE_SOCK (-1)
#define NO_USER (-1)


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

#endif
