/**
* UDP and TCP convenience functions, simplifying interactions with the network.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "shared.h"
#include "util/network.h"

#define MAX_PENDING 50

/**
 * Gets a socket
 *
 * @param address
 * @param timeout
 * @return
 */
int getSocket(const struct sockaddr_in *address, const struct timeval *timeout, enum ConnectionType connectionType) {
  const enum __socket_type sockType = connectionType == DATAGRAM ? SOCK_DGRAM : SOCK_STREAM;
  const int protocol = connectionType == DATAGRAM ? IPPROTO_UDP : IPPROTO_TCP;

  const int sock = socket(AF_INET, sockType, protocol);
  if (sock < 0) {
    perror("[ERROR] socket() failed");
    return ERROR;
  }

  if (connectionType == STREAM) {
    const int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  }

  if (timeout) {
    const int optResult = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, timeout, sizeof(*timeout));
    if (optResult < 0) {
      perror("[ERROR] Unable to set socket options");
      close(sock);
      return ERROR;
    }
  }

  if (address && bind(sock, (struct sockaddr *) address, sizeof(*address)) < 0) {
    perror("[ERROR] bind() failed");
    close(sock);
    return ERROR;
  }

  return sock;
}

/**
 * Gets a network address from an ASCII IP address and a port
 * @param ipAddress
 * @param serverPort
 * @return
 */
struct sockaddr_in getNetworkAddress(const char *ipAddress, const unsigned short serverPort) {
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  if (ipAddress) {
    inet_pton(AF_INET, ipAddress, &addr.sin_addr);
  } else {
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  addr.sin_port = htons(serverPort);
  printf("Constructing port: original=%u, byteswapped=%u\n", serverPort, addr.sin_port);
  return addr;
}

/**
 * Receives a message from a given socket
 *
 * @param socket
 * @param message
 * @param messageSize
 * @param clientAddress
 * @return
 */
int receiveUdpMessage(const int socket, char *message, const size_t messageSize,
                      struct sockaddr_in *clientAddress) {
  socklen_t clientAddrLen = sizeof(*clientAddress);
  const ssize_t numBytes = recvfrom(socket, message, messageSize, 0,
                                    (struct sockaddr *) clientAddress, &clientAddrLen);
  if (numBytes < 0) {
    perror("[ERROR] recvfrom() failed");
    return ERROR;
  }

  char printableAddress[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &clientAddress->sin_addr, printableAddress, INET_ADDRSTRLEN);

  if (numBytes != (ssize_t) messageSize) {
    printf("[ERROR] Received more bytes than expected: received %zd, expected %zd. Output is truncated.\n", numBytes,
           messageSize);
    return ERROR;
  }

  return SUCCESS;
}

/**
 * Sends a message on a given socket
 *
 * @param socket
 * @param messageBuffer
 * @param messageSize
 * @param destinationAddress
 * @return
 */
int sendUdpMessage(const int socket, const char *messageBuffer, const size_t messageSize,
                   const struct sockaddr_in *destinationAddress) {
  const ssize_t numBytes = sendto(socket, messageBuffer, messageSize, 0, (struct sockaddr *) destinationAddress,
                                  sizeof(*destinationAddress));

  printf("sendUdpMessage - %u, %u, %u\n", destinationAddress->sin_addr.s_addr,
    destinationAddress->sin_port, destinationAddress->sin_family);

  if (numBytes < 0) {

    printf("[ERROR] sendTo() failed, %d\n", errno);
    perror("[ERROR] sendTo() failed;");
    return ERROR;
  }

  if (numBytes != (ssize_t) messageSize) {
    printf("[ERROR] sendto() sent a different number of bytes than expected\n");
    return ERROR;
  }

  return SUCCESS;
}

int tcpConnect(const int sock, const struct sockaddr_in *serverAddress) {
  if (connect(sock, (struct sockaddr *) serverAddress, sizeof(struct sockaddr_in)) < 0) {
    printf("[ERROR] Unable to connect to host\n");
    return ERROR;
  }
  return SUCCESS;
}

int tcpListen(const int sock) {
  printf("[DEBUG] Attempting to listen...\n");
  if (listen(sock, MAX_PENDING) < 0) {
    perror("[ERROR] Failed to listen");
    return ERROR;
  }
  printf("Listen initiated!\n");
  return SUCCESS;
}

int tcpAccept(const int sock, struct sockaddr_in *clientAddress, int *clientSock) {
  socklen_t clientLength = sizeof(struct sockaddr_in);
  printf("[DEBUG] Attempting to accept...\n");
  if ((*clientSock = accept(sock, (struct sockaddr *) clientAddress, &clientLength)) < 0) {
    perror("[ERROR] Failed to accept");
    return ERROR;
  }
  printf("[DEBUG] Accepted!\n");
  return SUCCESS;
}

int sendTcpMessage(const int socket, const char *messageBuffer, const size_t messageSize) {
  const ssize_t numBytes = send(socket, messageBuffer, messageSize, 0);

  if (numBytes < 0) {
    perror("[ERROR] Stream send() failed");
    return ERROR;
  }

  if (numBytes != (ssize_t) messageSize) {
    perror("[ERROR] Stream send() sent a different number of bytes than expected");
    return ERROR;
  }

  return SUCCESS;
}

int receiveTcpMessage(const int socket, char *message, const size_t messageSize) {
  char *tempBuffer = malloc(messageSize);
  size_t offset = 0;
  ssize_t numBytes = recv(socket, tempBuffer, messageSize, 0);

  while (numBytes > 0) {
    memcpy(message + offset, tempBuffer, numBytes);
    offset += numBytes;
    if (offset == messageSize) {
      break;
    }
    numBytes = recv(socket, tempBuffer, messageSize, 0);
  }

  int ret = SUCCESS;
  if (numBytes < 0) {
    if (errno == EINTR) {
      ret = TERMINATED;
    } else {
      perror("[ERROR] Stream recv() failed - likely a timeout");
      ret = ERROR;
    }
  } else if (numBytes == 0) {
    ret = TERMINATED;
  }
  free(tempBuffer);
  return ret;
}
