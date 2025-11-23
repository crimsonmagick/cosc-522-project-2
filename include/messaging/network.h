/**
* Interface for UDP convenience functions, simplifying interactions with the network.
*/

#ifndef COSC522_LODI_MESSAGING_H
#define COSC522_LODI_MESSAGING_H
#include <arpa/inet.h>

#define LOCALHOST "127.0.0.1"

enum ConnectionType {
  STREAM, DATAGRAM
};

int getSocket(const struct sockaddr_in *address, const struct timeval *timeout, enum ConnectionType connectionType);

struct sockaddr_in getNetworkAddress(const char *ipAddress, const unsigned short serverPort);

int receiveUdpMessage(const int socket, char *message, const size_t messageSize, struct sockaddr_in *clientAddress);

int sendUdpMessage(const int socket, const char *messageBuffer, const size_t messageSize,
                const struct sockaddr_in *destinationAddress);

int tcpConnect(const int sock, const struct sockaddr_in *serverAddress);

int tcpListen(const int sock);

int tcpAccept(const int sock, struct sockaddr_in *clientAddress, int* clientSock);

int sendTcpMessage(const int socket, const char *messageBuffer, const size_t messageSize);

int receiveTcpMessage(const int socket, char *message, const size_t messageSize);

#endif //COSC522_LODI_MESSAGING_H
