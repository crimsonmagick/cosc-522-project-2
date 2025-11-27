#include "print_feed.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "shared.h"
#include "domain/lodi.h"

int handle_feed(unsigned int userId, unsigned long timestamp,
                unsigned digitalSig) {
  printf("Initializing feed...\n");
  DomainClient *client;
  if (initLodiClientDomain(&client) == DOMAIN_FAILURE) {
    printf("Failed to initialize Lodi Client when requesting feed!\n");
    exit(-1);
  }
  client->base.start(&client->base);

  PClientToLodiServer request = {
    .messageType = feed,
    .userID = userId,
    .timestamp = timestamp,
    .digitalSig = digitalSig
  };

  if (client->send(client, (UserMessage *) &request) == ERROR) {
    printf("Failed to send feed request... Exiting...\n");
    exit(-1);
  }

  while (true) {
    LodiServerMessage response;

    if (client->receive(client, (UserMessage *) &response) == DOMAIN_FAILURE) {
      printf("Failed to receive feed message...\n");
    } else {
      printf("Received feed message: idolId=%u, message=%s", response.recipientID, response.message);
    }
  }
}
