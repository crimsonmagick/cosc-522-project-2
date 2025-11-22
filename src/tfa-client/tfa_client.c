/**
* This source file implements the "TFA Client" functionality:
 *
 * 1) Registers clients IP address and port in the TFA Server
 * 2  Responds to push authentication requests sent from the Lodi Server
 **/

#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <time.h>
#include <stdbool.h>

#include "domain_datagram.h"
#include "messaging/tfa_messaging.h"
#include "shared.h"
#include "util/rsa.h"
#include "util/server_configs.h"

#define REGISTER_OPTION 1
#define LOGIN_OPTION 2
#define QUIT_OPTION 3

int getMainOption();

unsigned long getLongInput(char *inputName);

int registerTFAClient(const unsigned int userID, unsigned long timestamp, unsigned long digitalSignature);

int lodiLogin(unsigned int userID, long timestamp, long digitalSignature);

void handleTFAPush();

static DomainService *tfaClientDomain = NULL;
static struct sockaddr_in tfaServerAddr;

/**
 * Main loop for TFA Client
 *
 * @return theoretically, 0 on success - loops forever in order to receive pushes after registration
 */
int main() {
    initTFAClientDomain(&tfaClientDomain,true);
    tfaServerAddr = getServerAddr(TFA);

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
 * Generic method for retrieving a long integer from keyboard input.
 * FIXME - this is duplicated from Lodi Client, needs to be extracted into a shared function
 *
 * @param inputName The name of the input you're prompting for
 * @return The long value input by the user
 */
unsigned long getLongInput(char *inputName) {
    long input = -1;
    while (input < 0) {
        printf("Please enter your %s:\n", inputName);
        char line[64];

        if (fgets(line, sizeof(line), stdin)) {
            sscanf(line, "%ld", &input);
            if (sscanf(line, "%d", &input) != 1 || input < 0) {
                printf("Invalid %s entered. Please try again!\n", inputName);
            }
        } else {
            printf("Failed to read user input. Please try again:\n");
        }
    }

    return (unsigned long) input;
}

/**
 * Handles push auth TFA requests - fulfills Req B. 2.
 */
void handleTFAPush() {
    while (true) {
        struct sockaddr_in clientAddress;

        TFAServerToTFAClient pushRequest;

        changeTimeout(tfaClientDomain, 0); // block
        int receivedSuccess = fromDatagramDomainHost(tfaClientDomain, &pushRequest, &clientAddress);

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
        changeTimeout(tfaClientDomain, DEFAULT_TIMEOUT_MS); // timeout on send

        int sendStatus = toDatagramDomainHost(tfaClientDomain, &toSendMessage, &tfaServerAddr);

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

    int sendStatus = toDatagramDomainHost(tfaClientDomain, &requestMessage, &tfaServerAddr);

    if (sendStatus == DOMAIN_FAILURE) {
        printf("Failed to send registration, aborting registration...\n");
        return ERROR;
    }
    printf("Req B. 1., a. and b. Sent registerTFA message with timestamp=%lu, digitalSignature=%lu\n",
           timestamp, digitalSignature);

    TFAServerToTFAClient response;
    struct sockaddr_in clientAddr;
    int receiveStatus = fromDatagramDomainHost(tfaClientDomain, &response, &clientAddr);
    if (receiveStatus == DOMAIN_FAILURE) {
        printf("Failed to receive registration, aborting registration...\n");
        return ERROR;
    }
    printf("Req B. 1. c. TFA registration request was received! Client got back: messageType=%u, userID=%u\n",
           response.messageType, response.userID);

    // AS PER REQUIREMENTS, SEND CONFIRMATION AGAIN
    requestMessage.messageType = ackRegTFA;
    sendStatus = toDatagramDomainHost(tfaClientDomain, &requestMessage, &tfaServerAddr);
    if (sendStatus == ERROR) {
        printf("Key registration failed while sending ack...\n");
        return ERROR;
    }
    printf("Req B. 1. d. Key registration ack sent successful!\n");

    receiveStatus = fromDatagramDomainHost(tfaClientDomain, &response, &clientAddr);
    if (receiveStatus == DOMAIN_FAILURE) {
        printf("Failed to receive final registration confirmation, aborting registration...\n");
        return ERROR;
    }

    printf(
        "Extra synchronous step - TFA registration successful and finally complete! Received: messageType=%u, userID=%u\n",
        response.messageType, response.userID);

    return sendStatus;
}
