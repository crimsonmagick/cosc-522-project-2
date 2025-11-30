/**
* This source file implements the "TFA Server" functionality:
 *
 *   1)  Registers client ip addresses and ports
 *   2)  Handles login authentication requests from the Lodi server
 *     i) Interfaces with registered TFA Clients to confirm the push authentication
 **/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "domain/tfa.h"
#include "domain/pke.h"
#include "registration_repository.h"
#include "shared.h"
#include "util/rsa.h"

static DomainClient *pkeClient = NULL;
static DomainServer *tfaServer = NULL;

void handleRegisterTfa(TFAClientOrLodiServerToTFAServer *request, ClientHandle *clientHandle) {
    unsigned int publicKey;
    getPublicKey(pkeClient, clientHandle->userID, &publicKey);
    printf("Req 1. a. 2) a) and b) - sent requestPublicKey and got responsePublicKey\n");

    // GOT PUBLIC KEY, NOW AUTHENTICATE
    const unsigned long digitalSig = request->digitalSig;
    const unsigned long timestamp = request->timestamp;
    const unsigned long decryptedTimestamp = decryptTimestamp(digitalSig, publicKey, MODULUS);
    if (decryptedTimestamp != timestamp) {
        printf("Authentication failed! Aborting TFA client registration...\n");
        // rely on client timing out...
        return;
    }
    printf("Req C. 1. a. 1) Authentication succeeded! Continuing with TFA client registration!\n");
    printf("Please note that C. 1. a. 2) must take place first (retrieve publicKey).\n");

    // WE AUTHENTICATED! SEND SUCCESS MESSAGE TO CLIENT
    TFAServerToTFAClient registrationSuccessMessage = {
        confirmTFA,
        clientHandle->userID,
    };

    if (tfaServer->send(tfaServer, (UserMessage *) &registrationSuccessMessage, clientHandle) == ERROR) {
        printf("Error while sending message.\n");
        return;
    }

    printf("Req C 1. a. 2) b. (repeated) sent confirmTFA\n");

    if (tfaServer->receive(tfaServer, (UserMessage *) request, clientHandle) == ERROR) {
        printf("Failed to handle incoming ACK TFAClientOrLodiServerToTFAServer message.\n");
        return;
    }

    if (request->messageType != ackRegTFA) {
        printf("Did not receive expected ack register message, aborting registration...\n");
        return;
    }
    printf("Req C 1. a. 2) c. Received expected ack register message! Finishing registration.\n");

    // REGISTER CLIENT
    addIP(request->userID, clientHandle->clientAddr.sin_addr, ntohs(clientHandle->clientAddr.sin_port));
    printf("Registered client! Sending final TFA confirmation message! (not in reqs)\n");
    if (tfaServer->send(tfaServer, (UserMessage *) &registrationSuccessMessage, clientHandle) == ERROR) {
        printf("Warning: error while sending final ack message for client registration.\n");
    }
}

void handlePushTfa(TFAClientOrLodiServerToTFAServer *request, ClientHandle *clientHandle) {
    printf("Req C. 2. a. received requestAuth message\n");
    struct in_addr registeredAddress;
    unsigned short port;
    if (getIP(request->userID, &registeredAddress, &port) == ERROR) {
        printf("IP Address was not registered with TFA server! Aborting auth request...\n");
        return;
    }

    TFAServerToTFAClient pushRequest = {
        .messageType = pushTFA,
        request->userID,
    };

    // FIXME we shouldn't be doing pure UDP stuff here...
    struct sockaddr_in tfaClientAddr = {0};
    tfaClientAddr.sin_family = AF_INET;
    tfaClientAddr.sin_addr = registeredAddress;
    tfaClientAddr.sin_port = htons(port);

    ClientHandle tfaClientHandle = {
        .userID = request->userID,
        .clientAddr = tfaClientAddr
    };

    int sendSuccess = tfaServer->send(tfaServer, (UserMessage *) &pushRequest, &tfaClientHandle);
    if (sendSuccess == ERROR) {
        printf("Failed to send push auth request to TFA client, aborting...\n");
        return;
    }
    printf("Req C. 3. a. sent pushTFA message\n");

    TFAClientOrLodiServerToTFAServer pushResponse;
    int receivedSuccess = tfaServer->receive(tfaServer, (UserMessage *) &pushResponse, &tfaClientHandle);
    if (receivedSuccess == ERROR) {
        printf("Error while receiving TFA client push auth message.\n");
        return;
    }

    if (pushResponse.messageType != ackPushTFA) {
        printf("Did not receive expected ack push message, aborting push auth...\n");
        return;
    }
    printf("Req C. 3. b. received ackPushTFA message\n");

    TFAServerToLodiServer pushNotificationResponse = {
        responseAuth,
        request->userID
    };
    sendSuccess = tfaServer->send(tfaServer, (UserMessage *) &pushNotificationResponse, clientHandle);
    if (sendSuccess == ERROR) {
        printf("Error while sending push response to Lodi server\n");
    }
    printf("Req C. 2. .b sent responseAuth message\n");
}

/**
 * Giant main function for TFA Server.
 * Fulfills all of Req 3.
 * @return nothing, loops forever and ever
 */
int main() {
    if (initPkeClient(&pkeClient) == ERROR
        || pkeClient->base.start(&pkeClient->base) == ERROR
        || initTFAServerDomain(&tfaServer) == ERROR
        || tfaServer->base.start(&tfaServer->base) == ERROR) {
        printf("Error while initializing PKE Server\n");
        exit(ERROR);
    }
    initRegistrationRepository();

    while (true) {
        TFAClientOrLodiServerToTFAServer request;
        ClientHandle clientHandle;
        if (tfaServer->receive(tfaServer, (UserMessage *) &request, &clientHandle) == DOMAIN_FAILURE) {
            printf("Failed to handle incoming message, continuing...\n");
        } else if (request.messageType == registerTFA) {
            handleRegisterTfa(&request, &clientHandle);
        } else if (request.messageType == requestAuth) {
            handlePushTfa(&request, &clientHandle);
        }
    }
}
