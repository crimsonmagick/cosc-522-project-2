/**
* This source file implements the "TFA Client" functionality:
 *
 * 1) Registers clients IP address and port in the TFA Server
 * 2  Responds to push authentication requests sent from the Lodi Server
 **/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include "domain/tfa.h"
#include "shared.h"
#include "util/input.h"
#include "util/rsa.h"

#define REGISTER_OPTION 1
#define LOGIN_OPTION 2
#define QUIT_OPTION 3

int getMainOption();

int registerTFAClient(const unsigned int userID, unsigned long timestamp, unsigned long digitalSignature);

int lodiLogin(unsigned int userID, long timestamp, long digitalSignature);

void handleTFAPush();

static DomainClient *tfaClient = NULL;

/**
 * Main loop for TFA Client
 *
 * @return theoretically, 0 on success - loops forever in order to receive pushes after registration
 */
int main() {
    initTFAClientDomain(&tfaClient);
    tfaClient->base.start(&tfaClient->base);

    printf("Welcome to the TFA Client!\n");
    unsigned int userID = getLongInput("user ID");
    unsigned long privateKey = getLongInput("private key");
    unsigned long timestamp;
    unsigned long digitalSignature;

    time(&timestamp);
    digitalSignature = encryptTimestamp(timestamp, privateKey, MODULUS);

    int registerStatus = registerTFAClient(userID, timestamp, digitalSignature);
    if (registerStatus == ERROR) {
        exit(1);
    }

    handleTFAPush();
}

/**
 * Handles push auth TFA requests - fulfills Req B. 2.
 */
void handleTFAPush() {
    while (true) {
        TFAServerToTFAClient pushRequest;

        tfaClient->base.changeTimeout(&tfaClient->base, 0); // block
        int receivedSuccess = tfaClient->receive(tfaClient, (UserMessage *) &pushRequest);

        if (receivedSuccess == ERROR) {
            printf("Failed to handle incoming TFAClientOrLodiServerToTFAServer message.\n");
            continue;
        }

        if (pushRequest.messageType != pushTFA) {
            printf("Received non pushTFA messaging... discarding and continuing...\n");
        }

        printf("Req. B. 2. a. Received pushTFA message\n");
        TFAClientOrLodiServerToTFAServer toSendMessage = {
            .messageType = ackPushTFA,
            .userID = pushRequest.userID,
            .timestamp = 0,
            .digitalSig = 0
        };
        tfaClient->base.changeTimeout(&tfaClient->base, DEFAULT_TIMEOUT_MS); // timeout on send

        int sendStatus = tfaClient->send(tfaClient, (UserMessage *) &toSendMessage);

        if (sendStatus == ERROR) {
            printf("Error while sending push ack.\n");
        } else {
            printf("Req B. 2. b. Responded to push auth request successfully with ackPushTFA!\n");
        }
    }
}

/**
 * Registers the TFA client's port and IP address, fulfilling Req B. 1
 *
 * @param userID user that's registering their "device"
 * @param timestamp nonce
 * @param digitalSignature used by TFA server to authenticate user
 * @return
 */
int registerTFAClient(const unsigned int userID, unsigned long timestamp, unsigned long digitalSignature) {
    printf("Registring IP Address and port\n");
    TFAClientOrLodiServerToTFAServer requestMessage = {
        .messageType = registerTFA,
        .userID = userID,
        .timestamp = timestamp,
        .digitalSig = digitalSignature
    };

    int sendStatus = tfaClient->send(tfaClient, (UserMessage *) &requestMessage);

    if (sendStatus == DOMAIN_FAILURE) {
        printf("Failed to send registration, aborting registration...\n");
        return ERROR;
    }
    printf("Req B. 1., a. and b. Sent registerTFA message with timestamp=%lu, digitalSignature=%lu\n",
           timestamp, digitalSignature);

    TFAServerToTFAClient response;
    int receiveStatus = tfaClient->receive(tfaClient, (UserMessage *) &response);
    if (receiveStatus == DOMAIN_FAILURE) {
        printf("Failed to receive registration, aborting registration...\n");
        return ERROR;
    }
    printf("Req B. 1. c. TFA registration request was received! Client got back: messageType=%u, userID=%u\n",
           response.messageType, response.userID);

    // AS PER REQUIREMENTS, SEND CONFIRMATION AGAIN
    requestMessage.messageType = ackRegTFA;
    sendStatus = tfaClient->send(tfaClient, (UserMessage *) &requestMessage);
    if (sendStatus == ERROR) {
        printf("Key registration failed while sending ack...\n");
        return ERROR;
    }
    printf("Req B. 1. d. Key registration ack sent successful!\n");

    receiveStatus = tfaClient->receive(tfaClient, (UserMessage *) &response);
    if (receiveStatus == DOMAIN_FAILURE) {
        printf("Failed to receive final registration confirmation, aborting registration...\n");
        return ERROR;
    }

    printf(
        "Extra synchronous step - TFA registration successful and finally complete! Received: messageType=%u, userID=%u\n",
        response.messageType, response.userID);

    return sendStatus;
}
