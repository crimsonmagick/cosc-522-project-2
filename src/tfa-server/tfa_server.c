/**
* This source file implements the "TFA Server" functionality:
 *
 *   1)  Registers client ip addresses and ports
 *   2)  Handles login authentication requests from the Lodi server
 *     i) Interfaces with registered TFA Clients to confirm the push authentication
 **/

#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "shared.h"
#include "registration_repository.h"
#include "messaging/tfa_messaging.h"
#include "messaging/pke_messaging.h"
#include "util/rsa.h"
#include "util/server_configs.h"

static DomainClient *pkeClient = NULL;
static DomainService *tfaDomain = NULL;
static struct sockaddr_in pkServerAddr;


/**
 * Giant main function for TFA Server. TODO break into smaller functions
 * Fulfills all of Req 3.
 * @return nothing, loops forever and ever
 */
int main() {
    // initialize domains
    initPKEClientDomain(&pkeClient);
    initTFAServerDomain(&tfaDomain);
    pkServerAddr = getServerAddr(PK);
    // initialize repository
    initRepository();

    while (true) {
        struct sockaddr_in clientAddress;

        TFAClientOrLodiServerToTFAServer receivedMessage;
        if (fromDatagramDomainHost(tfaDomain, &receivedMessage, &clientAddress) == DOMAIN_FAILURE) {
            printf("Failed to handle incoming message, continuing...\n");
        }

        if (receivedMessage.messageType == registerTFA) {
            unsigned int publicKey;
            getPublicKey(pkeClient, &pkServerAddr, receivedMessage.userID, &publicKey);
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

            int sendSuccess = toDatagramDomainHost(tfaDomain, &registrationSuccessMessage, &clientAddress);

            if (sendSuccess == ERROR) {
                printf("Error while sending message.\n");
                continue;
            }

            printf("Req C 1. a. 2) b. (repeated) sent confirmTFA\n");

            // RECEIVE ACK MESSAGE OF DESTINY
            int receivedSuccess = fromDatagramDomainHost(tfaDomain, &receivedMessage, &clientAddress);

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
            addIP(receivedMessage.userID, clientAddress.sin_addr, ntohs(clientAddress.sin_port));
            printf("Registered client! Sending final TFA confirmation message! (not in reqs)\n");
            sendSuccess = toDatagramDomainHost(tfaDomain, &registrationSuccessMessage, &clientAddress);
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

            int sendSuccess = toDatagramDomainHost(tfaDomain, &pushRequest, &tfaClientAddr);
            if (sendSuccess == ERROR) {
                printf("Failed to send push auth request to TFA client, aborting...\n");
                continue;
            }
            printf("Req C. 3. a. sent pushTFA message\n");

            TFAClientOrLodiServerToTFAServer pushResponse;
            int receivedSuccess = fromDatagramDomainHost(tfaDomain, &pushResponse, &tfaClientAddr);
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
            sendSuccess = toDatagramDomainHost(tfaDomain, &pushNotificationResponse, &clientAddress);
            if (sendSuccess == ERROR) {
                printf("Error while sending push response to Lodi server\n");
            }
            printf("Req C. 2. .b sent responseAuth message\n");
        }
    }
}
