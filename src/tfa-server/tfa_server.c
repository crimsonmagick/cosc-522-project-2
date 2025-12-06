/**
* This source file implements the "TFA Server" functionality:
 *
 *   1)  Registers client ip addresses and ports
 *   2)  Handles login authentication requests from the Lodi server
 *     i) Interfaces with registered TFA Clients to confirm the push authentication
 **/

#include <stdio.h>
#include <stdlib.h>

#include "domain/tfa.h"
#include "domain/pke.h"
#include "registration_repository.h"
#include "shared.h"
#include "util/rsa.h"

void handleRegisterTfa(TFAClientOrLodiServerToTFAServer *request, ClientHandle *clientHandle);

void handlePushTfa(TFAClientOrLodiServerToTFAServer *request, ClientHandle *clientHandle);

static DomainClient *pkeClient = NULL;
static DomainServer *tfaServer = NULL;

/**
 * Giant main function for TFA Server.
 */
int main() {
    if (initPkeClient(&pkeClient) == ERROR
        || pkeClient->base.start(&pkeClient->base) == ERROR
        || initTFAServerDomain(&tfaServer) == ERROR
        || tfaServer->base.start(&tfaServer->base) == ERROR) {
        printf("Error while initializing TFA Server\n");
        exit(ERROR);
    }

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

void handleRegisterTfa(TFAClientOrLodiServerToTFAServer *request, ClientHandle *clientHandle) {
    unsigned int publicKey;

    TFAServerToTFAClient response = {
        confirmTFA,
        clientHandle->userID,
    };
    tfaServer->base.changeTimeout(&tfaServer->base, DEFAULT_TIMEOUT_MS);
    if (getPublicKey(pkeClient, clientHandle->userID, &publicKey) == ERROR) {
        printf("Failed to get public key from PKE server...\n");
        response.messageType = tfaFailure;
    } else if (decryptTimestamp( request->digitalSig, publicKey, MODULUS) != request->timestamp) {
        printf("Authentication failed! Aborting TFA client registration...\n");
        response.messageType = tfaFailure;
    } else if (tfaServer->send(tfaServer, (UserMessage *) &response, clientHandle) == ERROR) {
        printf("Error while sending initial auth message to TFA Client.\n");
        response.messageType = tfaFailure;
    } else if (tfaServer->receive(tfaServer, (UserMessage *) request, clientHandle) == ERROR) {
        printf("Failed to handle incoming ACK TFAClientOrLodiServerToTFAServer message.\n");
        response.messageType = tfaFailure;
    } else if (request->messageType != ackRegTFA) {
        printf("Did not receive expected ack register message, aborting registration...\n");
        response.messageType = tfaFailure;
    } else {
        printf("Received expected ack register message! Finishing registration.\n");
        registerClient(request->userID, clientHandle);
        printf("Registered client! Sending final TFA confirmation message!\n");
        response.messageType = confirmTFA;
    }
    if (tfaServer->send(tfaServer, (UserMessage *) &response, clientHandle) == ERROR) {
        printf("Warning: error while sending final message during client registration.\n");
    }
    tfaServer->base.changeTimeout(&tfaServer->base, 0);
}

void handlePushTfa(TFAClientOrLodiServerToTFAServer *request, ClientHandle *clientHandle) {
    printf("Received requestAuth message\n");
    ClientHandle *tfaClientHandle;
    if (getClient(request->userID, &tfaClientHandle) != SUCCESS) {
        printf("IP Address was not registered with TFA server! Aborting auth request...\n");
        return;
    }

    TFAServerToTFAClient pushRequest = {
        .messageType = pushTFA,
        request->userID,
    };
    int sendSuccess = tfaServer->send(tfaServer, (UserMessage *) &pushRequest, tfaClientHandle);
    if (sendSuccess == DOMAIN_FAILURE) {
        printf("Failed to send push auth request to TFA client, aborting...\n");
        return;
    }
    printf("sent pushTFA message\n");

    TFAClientOrLodiServerToTFAServer pushResponse;
    if (tfaServer->receive(tfaServer, (UserMessage *) &pushResponse, tfaClientHandle) == ERROR) {
        printf("Error while receiving TFA client push auth message.\n");
        return;
    }

    if (pushResponse.messageType != ackPushTFA) {
        printf("Did not receive expected ack push message, aborting push auth...\n");
        return;
    }
    printf("Received ackPushTFA message\n");

    TFAServerToLodiServer pushNotificationResponse = {
        responseAuth,
        request->userID
    };
    sendSuccess = tfaServer->send(tfaServer, (UserMessage *) &pushNotificationResponse, clientHandle);
    if (sendSuccess == ERROR) {
        printf("Error while sending push response to Lodi server\n");
    }
    printf("ResponseAuth message\n");
}
