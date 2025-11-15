/**
* UDP convenience functions, simplifying interactions with the network.
*/

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#include "shared.h"
#include "messaging/network.h"

#define TIMEOUT_SECONDS 0
#define SOCK_FAILURE (-1)

/**
 * Gets a UDP socket
 *
 * @param address
 * @param timeout
 * @return
 */
int getSocket(const struct sockaddr_in *address, const struct timeval *timeout, enum ConnectionType connectionType) {
  enum __socket_type sockType = connectionType == DATAGRAM ? SOCK_DGRAM : SOCK_STREAM;
  const int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0) {
    perror("socket() failed");
    return SOCK_FAILURE;
  }

  if (timeout) {
    const int optResult = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, timeout, sizeof(*timeout));
    if (optResult < 0) {
      perror("Unable to set socket options");
      close(sock);
      return SOCK_FAILURE;
    }
  }

  if (address && bind(sock, (struct sockaddr *) address, sizeof(*address)) < 0) {
    perror("bind() failed");
    close(sock);
    return SOCK_FAILURE;
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
int receiveMessage(const int socket, char *message, const size_t messageSize, struct sockaddr_in *clientAddress) {
  socklen_t clientAddrLen = sizeof(*clientAddress);
  const ssize_t numBytes = recvfrom(socket, message, messageSize, 0,
                                    (struct sockaddr *) clientAddress, &clientAddrLen);
  if (numBytes < 0) {
    perror("recvfrom() failed");
    return ERROR;
  }

  char printableAddress[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &clientAddress->sin_addr, printableAddress, INET_ADDRSTRLEN);
  printf("Received message from client %s\n", printableAddress);

  if (numBytes != (ssize_t) messageSize) {
    printf("Received more bytes than expected: received %zd, expected %zd. Output is truncated.\n", numBytes,
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
int sendMessage(const int socket, const char *messageBuffer, const size_t messageSize,
                const struct sockaddr_in *destinationAddress) {
  const ssize_t numBytes = sendto(socket, messageBuffer, messageSize, 0, (struct sockaddr *) destinationAddress,
                                sizeof(*destinationAddress));

  if (numBytes < 0) {
    perror("sendTo() failed");
    return ERROR;
  }

  if (numBytes != (ssize_t) messageSize) {
    printf("sendto() sent a different number of bytes than expected\n");
    return ERROR;
  }

  return SUCCESS;
}