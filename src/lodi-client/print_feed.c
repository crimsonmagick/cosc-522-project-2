#include "print_feed.h"

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/wait.h>

#include "shared.h"
#include "domain/lodi.h"

#define CONNECT_BACKOFF 3

static volatile sig_atomic_t isRunning = 0;

static int handleClientFeed(unsigned int userId, unsigned long timestamp,
                            unsigned digitalSig);

static void handleSigterm(int sig);

int startStreamFeed(unsigned int userId, unsigned int timestamp, unsigned int digitalsig) {
  int pid = fork();
  if (pid == 0) {
    prctl(PR_SET_PDEATHSIG, SIGTERM);
    struct sigaction sa;
    sa.sa_handler = handleSigterm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
    isRunning = 1;
    int handleRet = handleClientFeed(userId, timestamp, digitalsig);
    exit(handleRet);
  }
  return pid;
}

void stopStreamFeed(int pid) {
  printf("Killing stream child with pid=%d...\n", pid);
  kill(pid, SIGTERM);
  waitpid(pid, NULL, 0);
  printf("Killed child!\n");
}

static int handleClientFeed(unsigned int userId, unsigned long timestamp,
                            unsigned digitalSig) {
  printf("Initializing feed...\n");
  DomainClient *client;
  if (initLodiClientDomain(&client) == DOMAIN_FAILURE) {
    printf("Failed to initialize Lodi Client when requesting feed!\n");
    return ERROR;
  }

  PClientToLodiServer request = {
    .messageType = feed,
    .userID = userId,
    .timestamp = timestamp,
    .digitalSig = digitalSig
  };


  while (isRunning) {
    int clientRet;
    if (!client->isConnected) {
      client->base.start(&client->base);
      clientRet = client->send(client, (UserMessage *) &request);
      if (clientRet != SUCCESS) {
        printf("Failed to send feed request... Retrying in %d seconds...\n", CONNECT_BACKOFF);
        sleep(CONNECT_BACKOFF);
        continue;
      }
    }

    LodiServerMessage response;

    clientRet = client->receive(client, (UserMessage *) &response);
    if (clientRet == DOMAIN_FAILURE) {
      printf("Failed to receive feed message...\n");
    } else if (clientRet == DOMAIN_SUCCESS) {
      printf("Received feed message: idolId=%u, message=%s", response.recipientID, response.message);
    } else if (clientRet == TERMINATED) {
      client->base.stop(&client->base);
      printf("Connection has been terminated, will attempt to reconnect in %d seconds.\n", CONNECT_BACKOFF);
      sleep(CONNECT_BACKOFF);
    } else {
      printf("Impossible state... Exiting...\n");
      return ERROR;
    }
  }

  client->base.destroy((DomainService **) &client);
  printf("Closed gracefully...\n");
  return SUCCESS;
}

static void handleSigterm(const int sig) {
  (void) sig;
  printf("[FEED-CHILD] received shutdown signal, exiting...\n");
  isRunning = 0;
}
