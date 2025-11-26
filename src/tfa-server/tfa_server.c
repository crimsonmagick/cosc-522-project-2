/**
* This source file implements the "TFA Server" functionality:
 *
 *   1)  Registers client ip addresses and ports
 *   2)  Handles login authentication requests from the Lodi server
 *     i) Interfaces with registered TFA Clients to confirm the push authentication
 **/

#include <stdio.h>
#include <arpa/inet.h>
#include <stdbool.h>

#include "shared.h"
#include "registration_repository.h"
#include "domain/tfa.h"
#include "../../include/domain/pke.h"
#include "util/rsa.h"
#include "util/server_configs.h"

static DomainClient *pkeClient = NULL;
static DomainServer *tfaServer = NULL;
static struct sockaddr_in pkServerAddr;


/**
 * Giant main function for TFA Server. TODO break into smaller functions
 * Fulfills all of Req 3.
 * @return nothing, loops forever and ever
 */
int main() {
    // initialize domains
    initPKEClientDomain(&pkeClient);
    pkeClient->base.start(&pkeClient->base);
    initTFAServerDomain(&tfaServer);
    tfaServer->base.start(&tfaServer->base);
    pkServerAddr = getServerAddr(PK);
    // initialize repository
    initRepository();

    while (true) {

        DomainHandle receiveHandle;
        TFAClientOrLodiServerToTFAServer receivedMessage;
        if (tfaServer->receive(tfaServer, (UserMessage *) &receivedMessage, &receiveHandle) == DOMAIN_FAILURE) {
            printf("Failed to handle incoming message, continuing...\n");
        }

        if (receivedMessage.messageType == registerTFA) {
            unsigned int publicKey;
            getPublicKey(pkeClient, receivedMessage.userID, &publicKey);
            printf("Req 1. a. 2) a) and b) - sent requestPublicKey and got responsePublicKey\n");

            // GOT PUBLIC KEY, NOW AUTHENTICATE
            const unsigned long digitalSig = receivedMessage.digitalSig;
            const unsigned long timestamp = receivedMessage.timestamp;
            const unsigned long decryptedTimesstamp = decryptTimestamp(digitalSig, publicKey, MODULUS);
            if (decryptedTimesstamp != timestamp) {
                printf("Authentication failed! Aborting TFA client registration...\n");
                // rely on client timing out...
                continue;
            }
            printf("Req C. 1. a. 1) Authentication succeeded! Continuing with TFA client registration!\n");
            printf("Please note that C. 1. a. 2) must take place first (retrieve publicKey).\n");

            // WE AUTHENTICATED! SEND SUCCESS MESSAGE TO CLIENT
            TFAServerToTFAClient registrationSuccessMessage = {
                confirmTFA,
                receivedMessage.userID,
            };

            int sendSuccess = tfaServer->send(tfaServer, (UserMessage *) &registrationSuccessMessage, &receiveHandle);

            if (sendSuccess == ERROR) {
                printf("Error while sending message.\n");
                continue;
            }

            printf("Req C 1. a. 2) b. (repeated) sent confirmTFA\n");

            // RECEIVE ACK MESSAGE OF DESTINY
            int receivedSuccess = tfaServer->receive(tfaServer, (UserMessage *) &receivedMessage, &receiveHandle);

            if (receivedSuccess == ERROR) {
                printf("Failed to handle incoming ACK TFAClientOrLodiServerToTFAServer message.\n");
                continue;
            }

            if (receivedMessage.messageType != ackRegTFA) {
                printf("Did not receive expected ack register message, aborting registration...\n");
                continue;
            }
            printf("Req C 1. a. 2) c. Received expected ack register message! Finishing registration.\n");

            // REGISTER CLIENT
            addIP(receivedMessage.userID, receiveHandle.host.sin_addr, ntohs(receiveHandle.host.sin_port));
            printf("Registered client! Sending final TFA confirmation message! (not in reqs)\n");
            sendSuccess = tfaServer->send(tfaServer, (UserMessage *) &registrationSuccessMessage, &receiveHandle);
            if (sendSuccess == ERROR) {
                printf("Warning: error while sending final ack message for client registration.\n");
            }
        } else if (receivedMessage.messageType == requestAuth) {
            printf("Req C. 2. a. received requestAuth message\n");
            struct in_addr registeredAddress;
            unsigned short port;
            if (getIP(receivedMessage.userID, &registeredAddress, &port) == ERROR) {
                printf("IP Address was not registered with TFA server! Aborting auth request...\n");
                continue;
            }

            TFAServerToTFAClient pushRequest = {
                .messageType = pushTFA,
                receivedMessage.userID,
            };

            // FIXME we shouldn't be doing pure UDP stuff here...
            struct sockaddr_in tfaClientAddr = {0};
            tfaClientAddr.sin_family = AF_INET;
            tfaClientAddr.sin_addr = registeredAddress;
            tfaClientAddr.sin_port = htons(port);

            DomainHandle tfaClientHandle = {
                .userID = receivedMessage.userID,
                .host = tfaClientAddr
            };

            int sendSuccess = tfaServer->send(tfaServer, (UserMessage *) &pushRequest, &tfaClientHandle);
            if (sendSuccess == ERROR) {
                printf("Failed to send push auth request to TFA client, aborting...\n");
                continue;
            }
            printf("Req C. 3. a. sent pushTFA message\n");

            TFAClientOrLodiServerToTFAServer pushResponse;
            int receivedSuccess = tfaServer->receive(tfaServer, (UserMessage *) &pushResponse, &tfaClientHandle);
            if (receivedSuccess == ERROR) {
                printf("Error while receiving TFA client push auth message.\n");
                continue;
            }

            if (pushResponse.messageType != ackPushTFA) {
                printf("Did not receive expected ack push message, aborting push auth...\n");
                continue;
            }
            printf("Req C. 3. b. received ackPushTFA message\n");

            TFAServerToLodiServer pushNotificationResponse = {
                responseAuth,
                receivedMessage.userID
            };
            sendSuccess = tfaServer->send(tfaServer, (UserMessage *) &pushNotificationResponse, &receiveHandle);
            if (sendSuccess == ERROR) {
                printf("Error while sending push response to Lodi server\n");
            }
            printf("Req C. 2. .b sent responseAuth message\n");
        }
    }
}
