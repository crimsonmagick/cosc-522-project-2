/**
 * Provides interface for persisting and retrieving RSA public keys
 */

#ifndef COSC522_LODI_KEY_REPOSITORY_H
#define COSC522_LODI_KEY_REPOSITORY_H

/**
 * Constructor function
 */
void initMessageRepository();

/**
 * Persists a public key
 * @param userId
 * @param publicKey
 * @return ERROR, SUCCESS
 */
int addKey(unsigned int userId, unsigned int publicKey);

/**
 * Retrieves publicKey
 * @param userId user to retrieve for
 * @param publicKey  output, the public key
 * @return ERROR, SUCCESS
 */
int getKey(unsigned int userId, unsigned int *publicKey);

#endif
