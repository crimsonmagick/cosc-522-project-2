/**
* Interface for UDP and TCP convenience functions, simplifying interactions with the network.
*/

#ifndef COSC522_LODI_NETWORK_H
#define COSC522_LODI_NETWORK_H
#include <arpa/inet.h>

#define LOCALHOST "127.0.0.1"

enum ConnectionType {
  STREAM, DATAGRAM
};

int getSocket(const struct sockaddr_in *address, const struct timeval *timeout, enum ConnectionType connectionType);

struct sockaddr_in getNetworkAddress(const char *ipAddress, unsigned short serverPort);

int receiveUdpMessage(int socket, char *message, size_t messageSize, struct sockaddr_in *clientAddress);

int sendUdpMessage(int socket, const char *messageBuffer, size_t messageSize,
                   const struct sockaddr_in *destinationAddress);

int tcpConnect(int sock, const struct sockaddr_in *serverAddress);

int tcpListen(int sock);

int tcpAccept(int sock, struct sockaddr_in *clientAddress, int *clientSock);

int sendTcpMessage(int socket, const char *messageBuffer, size_t messageSize);

int receiveTcpMessage(int socket, char *message, size_t messageSize);

#endif
