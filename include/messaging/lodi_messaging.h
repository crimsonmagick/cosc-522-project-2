/**
* Interface for shared Lodi Server+Client functions for serializing/deserializing domain structs in a network-safe way, converting bytes to and from
 * big-endian.
 */

#ifndef LODI_LODIMESSAGING_H
#define LODI_LODIMESSAGING_H

#define LODI_MESSAGE_LENGTH 100

#define LODI_CLIENT_REQUEST_SIZE ((3 * sizeof(uint32_t) + 2 * sizeof(uint64_t)) + LODI_MESSAGE_LENGTH * sizeof(char))
#define LODI_SERVER_RESPONSE_SIZE ((2 * sizeof(uint32_t)) + LODI_MESSAGE_LENGTH * sizeof(char))
#include "domain_stream.h"

enum LodiClientMessageType {
  login, post, feed, follow, unfollow, logout
};

enum LodiServerMessageType {
  ackLogin, ackPost, ackFeed, ackFollow, ackUnfollow, ackLogout, failure
};

typedef struct {
  enum LodiServerMessageType messageType; /* same size as an unsigned int */
  unsigned int userID; /* user identifier */
  char message[100]; /* text message*/
} LodiServerMessage;

typedef struct {
  enum LodiClientMessageType messageType;

  unsigned int userID; /* user identifier */
  unsigned int recipientID; /* message recipient identifier */
  unsigned long timestamp; /* timestamp */
  unsigned long digitalSig; /* encrypted timestamp */
  char message[100]; /* text message*/
} PClientToLodiServer;

int initLodiClientDomain(StreamDomainServiceHandle **handle);

int initLodiServerDomain(StreamDomainServiceHandle **handle);
#endif
