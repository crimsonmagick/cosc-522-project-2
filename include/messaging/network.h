/**
* Interface for UDP convenience functions, simplifying interactions with the network.
*/

#ifndef COSC522_LODI_MESSAGING_H
#define COSC522_LODI_MESSAGING_H

#define LOCALHOST "127.0.0.1"

enum ConnectionType {
  STREAM, DATAGRAM
};

int getSocket(const struct sockaddr_in *address, const struct timeval *timeout, enum ConnectionType connectionType);

struct sockaddr_in getNetworkAddress(const char *ipAddress, const unsigned short serverPort);

int receiveDatagramMessage(const int socket, char *message, const size_t messageSize, struct sockaddr_in *clientAddress);

int sendDatagramMessage(const int socket, const char *messageBuffer, const size_t messageSize,
                const struct sockaddr_in *destinationAddress);

int streamConnect(const int sock, const struct sockaddr_in *serverAddress);

int streamListen(const int sock);

int streamAccept(const int sock, struct sockaddr_in *clientAddress, int* clientSock);

int sendStreamMessage(const int socket, const char *messageBuffer, const size_t messageSize);

int receiveStreamMessage(const int socket, char *message, const size_t messageSize);

#endif //COSC522_LODI_MESSAGING_H
