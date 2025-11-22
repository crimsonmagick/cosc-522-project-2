#ifndef COSC522_LODI_DOMAIN_H
#define COSC522_LODI_DOMAIN_H

#define DOMAIN_SUCCESS 0
#define MESSAGE_SERIALIZER_SUCCESS 0
#define MESSAGE_SERIALIZER_FAILURE 1
#define MESSAGE_DESERIALIZER_SUCCESS 0
#define MESSAGE_DESERIALIZER_FAILURE 1
#define DOMAIN_FAILURE 1
#define DOMAIN_INIT_FAILURE 2

#define DEFAULT_TIMEOUT_MS 0

typedef struct MessageSerializer {
  size_t messageSize;

  int (*serializer)(void *, char *);
} MessageSerializer;

typedef struct MessageDeserializer {
  size_t messageSize;

  int (*deserializer)(char *, void *);
} MessageDeserializer;

typedef struct {
  unsigned int messageType; /* placeholder for implementations */
  unsigned int userID; /* user identifier, common to all messages*/
  // struct may have arbitrary fields contiguously in memory after the userID
} UserMessage;


#endif