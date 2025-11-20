/**
 * This source file implements the "Lodi Client" functionality:
 *
 * 1) Registers user's public key against the PKE Server.
 *    See registerPublicKey() for the implementation of this functionality
 * 2) Authenticates against the Lodi Server
 *    See lodiLogin() for the implementation of this functionality.
 **/

#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>

#include "messaging/pke_messaging.h"
#include "messaging/lodi_messaging.h"
#include "shared.h"
#include "util/rsa.h"
#include "util/server_configs.h"

#define REGISTER_OPTION 1
#define LOGIN_OPTION 2
#define QUIT_OPTION 3

static DomainService *pkeDomain = NULL;
static StreamDomainServiceHandle *lodiDomain = NULL;
static struct sockaddr_in pkServerAddr;
static struct sockaddr_in lodiServerAddr;

int getMainOption();

unsigned long getLongInput(char *inputName);

int registerPublicKey(unsigned int userID, unsigned int publicKey);

int lodiLogin(unsigned int userID, long timestamp, long digitalSignature);

/**
 * Main loop for Lodi Client
 *
 * @return theoretically, 0 on success - loop uses exit() to end the program instead
 */
int main() {
    // initialize domains
    initPKEClientDomain(&pkeDomain);
    if (initLodiClientDomain(&lodiDomain) == DOMAIN_FAILURE) {
        printf("Failed to initialize Lodi Client Domain!\n");
        exit(-1);
    }
    pkServerAddr = getServerAddr(PK);
    lodiServerAddr = getServerAddr(LODI);

    printf("Welcome to the Lodi Client!\n");
    unsigned int userID = getLongInput("user ID");
    printf("Now choose from the following options:\n");

    int selected = 0;
    while (selected != QUIT_OPTION) {
        selected = getMainOption();

        unsigned long publicKey;
        unsigned long privateKey;
        unsigned long timestamp;
        unsigned long digitalSignature;
        switch (selected) {
            case REGISTER_OPTION:
                // TODO look into generating public/private key pair either here or as another option
                publicKey = getLongInput("public key");
                registerPublicKey(userID, publicKey);
                break;
            case LOGIN_OPTION:
                // TODO look into reusing generated private key
                privateKey = getLongInput("private key");
                time(&timestamp);
                digitalSignature = encryptTimestamp(timestamp, privateKey, MODULUS);
                lodiLogin(userID, timestamp, digitalSignature);
                break;
            case QUIT_OPTION:
                printf("See you later!\n");
                break;
            default:
                printf("Please enter a valid option: %d, %d, or %d\n",
                       REGISTER_OPTION, LOGIN_OPTION, QUIT_OPTION);
                break;
        }
    }

    exit(0);
}

/**
 * Gets an option from the user for the main Lodi client loop.
 *
 * @return a selected int: 1,2, or 3
 */
int getMainOption() {
    printf("1. Register your Lodi Key\n");
    printf("2. Login to Lodi\n");
    printf("3. Quit\n");

    int selected = 0;
    char line[64];

    if (fgets(line, sizeof(line), stdin)) {
        sscanf(line, "%d", &selected);
    }
    return selected;
}

/**
 * Gets user selected Lodi option after login. Fulfills A. 3.
 *
 * @return the selected option (it's gonna be 1!)
 */
int getLodiLoopOption() {
    printf("1. Post a message\n");
    printf("2. Logout\n");

    int selected = 0;
    char line[64];

    if (fgets(line, sizeof(line), stdin)) {
        sscanf(line, "%d", &selected);
    }
    return selected;
}

void getStringInput(char *inputName, char *inputStr, const int strLength) {
    bool inputSuccess = false;
    while (!inputSuccess) {
        printf("Please enter your %s:\n", inputName);

        if (!fgets(inputStr, strLength, stdin)) {
            printf("Failed to read user input. Please try again:\n");
        } else {
            inputSuccess = true;
        }
    }
}

/**
 * Generic method for retrieving a long integer from keyboard input.
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
 * Registers the publicKey against the PKE server. Fulfills requirement A. 1.
 *
 * @param userID User associated with the publicKey
 * @param publicKey The publicKey to be registered
 * @return SUCCESS or FAILURE (int)
 */
int registerPublicKey(const unsigned int userID, const unsigned int publicKey) {
    const PClientToPKServer requestMessage = {
        registerKey,
        userID,
        publicKey
    };

    if (toDatagramDomainHost(pkeDomain, (void *) &requestMessage, &pkServerAddr) == DOMAIN_FAILURE) {
        printf("Unable to send registration, aborting ...\n");
        return ERROR;
    }
    printf("Req A. 1. a. - sent registerKey Message to PKE Server\n");

    PKServerToLodiClient responseMessage;

    struct sockaddr_in receiveAddress;
    if (fromDatagramDomainHost(pkeDomain, &responseMessage, &receiveAddress) == DOMAIN_FAILURE) {
        printf("Failed to receive registration confirmation, aborting ...\n");
        return ERROR;
    }
    printf("Req A. 1. b. - wait to receive an ackRegisterKey message to confirm registration was successful.\n");
    printf("Req A. 1.b. message details: messageType=%u, userID=%u, publicKey=%u\n",
           responseMessage.messageType, responseMessage.userID, responseMessage.publicKey);

    return SUCCESS;
}


int lodiPost(const unsigned int userID, const long timestamp, const long digitalSignature) {
    const PClientToLodiServer request = {
        .messageType = post,
        .userID = userID,
        .recipientID = 0,
        .timestamp = timestamp,
        .digitalSig = digitalSignature
    };
    getStringInput("your Lodi message", request.message, 100);


    if (toStreamDomainHost(lodiDomain, (void *) &request) == ERROR) {
        printf("Failed to send Post...\n");
        return ERROR;
    }

    LodiServerMessage response;

    if (fromStreamDomainHost(lodiDomain, &response) == DOMAIN_FAILURE) {
        printf("Failed to receive Post response...\n");
        return ERROR;
    }

    printf("Received ack of post: messageType=%u, userID=%u\n",
           response.messageType, response.userID);
    return SUCCESS;
}

/**
 * Authenticates user, logging into the Lodi server. Fulfills requirement A. 2.
 *
 * @param userID The user attempting to authenticate
 * @param timestamp The unencrypted timestamp (nonce)
 * @param digitalSignature The encrypted timestamp
 * @return SUCCESS or FAILURE (int)
 */
int lodiLogin(const unsigned int userID, const long timestamp, const long digitalSignature) {
    const PClientToLodiServer request = {
        .messageType = login,
        .userID = userID,
        .recipientID = 0,
        .timestamp = timestamp,
        .digitalSig = digitalSignature,
        .message = "Hello from Lodi Client!"
    };

    if (toStreamDomainHost(lodiDomain, (void *) &request) == ERROR) {
        printf("Req A. c. Failed to send login message, aborting...\n");
        return ERROR;
    }

    LodiServerMessage response;

    if (fromStreamDomainHost(lodiDomain, &response) == DOMAIN_FAILURE) {
        printf("Req A. 2. c. Failed to receive login message, aborting...\n");
        return ERROR;
    }

    printf("Req A 2. b. 1. Login successful! Received: messageType=%u, userID=%u\n",
           response.messageType, response.userID);


    int selected = 0;
    while (true) {
        printf("Please select from our many amazing Lodi options:\n");
        selected = getLodiLoopOption();
        if (selected == 1) {
            lodiPost(userID, timestamp, digitalSignature);
            printf("Message posted successfully!\n");
        } else if (selected == 2) {
            // TODO send logout message
            break;
        } else {
            printf("Please enter a valid option\n");
        }
    }
    return SUCCESS;
}
