/*
 * See stream_feed.h
 */

#include "stream_feed.h"

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
                            unsigned long digitalSig);

static void handleSigterm(int sig);

int startStreamFeed(const unsigned int userId, const unsigned long timestamp, const unsigned long digitalSignature) {
  int pid = fork();
  if (pid == 0) {
    prctl(PR_SET_PDEATHSIG, SIGTERM);
    struct sigaction sa;
    sa.sa_handler = handleSigterm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
    isRunning = 1;
    const int handleRet = handleClientFeed(userId, timestamp, digitalSignature);
    exit(handleRet);
  }
  return pid;
}

void stopStreamFeed(const int pid) {
  printf("[FEED DEBUG] Killing stream child with pid=%d...\n", pid);
  kill(pid, SIGTERM);
  waitpid(pid, NULL, 0);
  printf("[FEED DEBUG] Killed child!\n");
}

/**
 * This fulfills the COSC 522 requirement - upon login, this separate thread dedicates itself to streaming messages from
 * the server for followed idols. The server immediately streams all existing followed idols' messages, and then streams
 * additional messages in real time as they're posted.
 *
 * @param userId User to start stream messages for
 * @param timestamp Login timestamp
 * @param digitalSig Login digital signature
 * @return
 */
static int handleClientFeed(const unsigned int userId, const unsigned long timestamp,
                            const unsigned long digitalSig) {
  printf("[FEED DEBUG] Initializing feed...\n");
  DomainClient *client;
  if (initLodiClient(&client) == DOMAIN_FAILURE) {
    printf("[FEED ERROR] Failed to initialize Lodi Client when requesting feed!\n");
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
      client->base.changeTimeout(&client->base, DEFAULT_TIMEOUT_MS);
      clientRet = client->send(client, (UserMessage *) &request);
      if (clientRet != SUCCESS) {
        printf("[FEED DEBUG] Failed to send feed request... Retrying in %d seconds...\n", CONNECT_BACKOFF);
        sleep(CONNECT_BACKOFF);
        continue;
      }
      client->base.changeTimeout(&client->base, 0);
    }

    LodiServerMessage response;

    clientRet = client->receive(client, (UserMessage *) &response);
    if (clientRet == DOMAIN_FAILURE) {
      printf("[FEED DEBUG] Failed to receive feed message...\n");
    } else if (clientRet == DOMAIN_SUCCESS) {
      printf("[FEED MESSAGE] [From Idol %u]: %s\n", response.recipientID, response.message);
      if (response.messageType == failure) {
        printf("[FEED DEBUG] Unrecoverable failure response from server. Please logout.\n");
        isRunning = 0;
      }
    } else if (clientRet == TERMINATED) {
      client->base.stop(&client->base);
      if (isRunning) {
        printf("[FEED DEBUG] Connection has been unexpectedly terminated, will attempt to reconnect in %d seconds.\n",
               CONNECT_BACKOFF);
        sleep(CONNECT_BACKOFF);
      }
    } else {
      printf("[FEED ERROR] Impossible state... Exiting...\n");
      isRunning = 0;
    }
  }

  client->base.destroy((DomainService **) &client);
  printf("[FEED DEBUG] Stopped message streaming gracefully...\n");
  return SUCCESS;
}

static void handleSigterm(const int sig) {
  if (sig == SIGTERM) {
    const char message[] = "[FEED DEBUG] received shutdown signal, exiting...\n";
    // avoid printf inside signal handlers
    write(STDOUT_FILENO, message, sizeof(message) - 1);
    isRunning = 0;
  }
}
