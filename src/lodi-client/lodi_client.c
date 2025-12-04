/**
 * This source file implements the "Lodi Client" functionality:
 *
 * 1) Registers user's public key against the PKE Server.
 *    See registerPublicKey() for the implementation of this functionality
 * 2) Authenticates against the Lodi Server
 *    See lodiLogin() for the implementation of this functionality.
 * 3) Implements logged-in Lodi messaging functionality (follow, unfollow, post, and logout)
 **/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "domain/pke.h"
#include "shared.h"
#include "util/input.h"
#include "util/rsa.h"

#include "lodi_client_domain_manager.h"
#include "stream_feed.h"

#define REGISTER_OPTION 1
#define LOGIN_OPTION 2
#define QUIT_OPTION 3

#define LODI_POST 1
#define LODI_FOLLOW 2
#define LODI_UNFOLLOW 3
#define LODI_LOGOUT 4

int registerPublicKey(unsigned int userID, unsigned int publicKey);

int lodiLogin(unsigned int userID, unsigned long timestamp, unsigned long digitalSignature);

/**
 * Main loop for Lodi Client
 *
 * @return theoretically, 0 on success - loop uses exit() to end the program instead
 */
int main() {
    printf("Welcome to the Lodi Client!\n");
    const unsigned int userID = getLongInput("user ID");
    printf("Now choose from the following options:\n");

    int selected = 0;
    while (selected != QUIT_OPTION) {
        printf("1. Register your Lodi Key\n");
        printf("2. Login to Lodi\n");
        printf("3. Quit\n");

        selected = getInt();

        fflush(stdout);

        unsigned long publicKey;
        unsigned long privateKey;
        time_t timestamp;
        unsigned long digitalSignature;
        switch (selected) {
            case REGISTER_OPTION:
                // TODO look into generating public/private key pair either here or as another option
                publicKey = getLongInput("public key");
                registerPublicKey(userID, publicKey);
                break;
            case LOGIN_OPTION:
                privateKey = getLongInput("private key");
                time(&timestamp);
                digitalSignature = encryptTimestamp(timestamp, privateKey, MODULUS);
                unsigned long decrypted = decryptTimestamp(digitalSignature, publicKey, MODULUS);
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

int lodiLogout(const unsigned int userID, const unsigned long timestamp, const unsigned long digitalSignature) {
    PClientToLodiServer request = {
        .messageType = logout,
        .userID = userID,
        .recipientID = 0,
        .timestamp = timestamp,
        .digitalSig = digitalSignature
    };

    LodiServerMessage response;

    const int status = lodiClientSend(&request, &response);
    if (status != DOMAIN_SUCCESS || response.messageType == failure) {
        printf("[MAIN DEBUG] Warning - logout failed. Were you already logged out?\n");
    } else {
        printf("[MAIN DEBUG] Logged out successfully.\n");
    }
    return status;
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
    PKServerToLodiClient responseMessage;
    const int status = lodiClientPkeSend(&requestMessage, &responseMessage);

    if (status != SUCCESS) {
        printf("Failed to receive registration confirmation, aborting ...\n");
    } else {
        printf("Message details: messageType=%u, userID=%u, publicKey=%u\n",
               responseMessage.messageType, responseMessage.userID, responseMessage.publicKey);
    }
    return status;
}

int lodiPost(const unsigned int userID, const unsigned long timestamp, const unsigned long digitalSignature) {
    PClientToLodiServer request = {
        .messageType = post,
        .userID = userID,
        .recipientID = 0,
        .timestamp = timestamp,
        .digitalSig = digitalSignature
    };
    getStringInput("your Lodi message", request.message, LODI_MESSAGE_LENGTH);

    LodiServerMessage response;

    const int status = lodiClientSend(&request, &response);
    if (status != DOMAIN_SUCCESS || response.messageType == failure) {
        printf("[MAIN DEBUG] Failed to Post message...\n");
    } else {
        printf("Message posted successfully!\n");
    }
    return status;
}

int lodiFollow(const unsigned int userID, const unsigned long timestamp, const unsigned long digitalSignature) {
    const unsigned int idolId = getLongInput("idolId");

    const PClientToLodiServer request = {
        .messageType = follow,
        .userID = userID,
        .recipientID = idolId,
        .timestamp = timestamp,
        .digitalSig = digitalSignature
    };
    LodiServerMessage response;

    int status = lodiClientSend(&request, &response);
    if (status != SUCCESS || response.messageType == failure) {
        printf("[MAIN DEBUG] Failed to send Follow message...\n");
    }
    return status;
}

int lodiUnfollow(const unsigned int userID, const unsigned long timestamp, const unsigned long digitalSignature) {
    const unsigned int idolId = getLongInput("idolId");

    const PClientToLodiServer request = {
        .messageType = unfollow,
        .userID = userID,
        .recipientID = idolId,
        .timestamp = timestamp,
        .digitalSig = digitalSignature
    };
    LodiServerMessage response;

    const int status = lodiClientSend(&request, &response);
    if (status != SUCCESS || response.messageType == failure) {
        printf("[MAIN DEBUG] Failed to send Unfollow message...\n");
    }
    return status;
}

/**
 * Authenticates user, logging into the Lodi server. Fulfills requirement A. 2.
 *
 * @param userID The user attempting to authenticate
 * @param timestamp The unencrypted timestamp (nonce)
 * @param digitalSignature The encrypted timestamp
 * @return SUCCESS or FAILURE (int)
 */
int lodiLogin(const unsigned int userID, const unsigned long timestamp, const unsigned long digitalSignature) {
    const PClientToLodiServer request = {
        .messageType = login,
        .userID = userID,
        .recipientID = 0,
        .timestamp = timestamp,
        .digitalSig = digitalSignature
    };

    LodiServerMessage response;

    const int status = lodiClientSend(&request, &response);
    if (status != DOMAIN_SUCCESS || response.messageType == failure) {
        printf("[MAIN DEBUG] Error: Login failed. Received: messageType=%u, userID=%u\n",
               response.messageType, response.userID);
        return status;
    }
    printf("[MAIN DEBUG] Login successful! Received: messageType=%u, userID=%u\n",
           response.messageType, response.userID);

    int selected = 0;
    const int pid = startStreamFeed(userID, timestamp, digitalSignature);
    if (pid < 0) {
        printf("[MAIN DEBUG] Failed to stream idol messages...\n");
        return ERROR;
    }

    while (true) {
        printf("Please select from our many amazing Lodi options:\n");
        printf("1. Post a message\n");
        printf("2. Follow an idol\n");
        printf("3. Unfollow an idol\n");
        printf("4. Logout\n");
        selected = getInt();
        switch (selected) {
            case LODI_POST:
                lodiPost(userID, timestamp, digitalSignature);
                break;
            case LODI_FOLLOW:
                lodiFollow(userID, timestamp, digitalSignature);
                break;
            case LODI_UNFOLLOW:
                lodiUnfollow(userID, timestamp, digitalSignature);
                break;
            case LODI_LOGOUT:
                stopStreamFeed(pid);
                lodiLogout(userID, timestamp, digitalSignature);
                return SUCCESS;
            default:
                printf("Please enter a valid option\n");
        }
    }
}
