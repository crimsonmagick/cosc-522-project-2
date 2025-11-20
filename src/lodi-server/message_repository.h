/**
* Provides persistence interface for registered client ip addresses and ports
 */

#ifndef COSC522_LODI_KEY_REPOSITORY_H
#define COSC522_LODI_KEY_REPOSITORY_H

void initRepository();

int addMessage(unsigned int userId, char *message);

int getMessages(unsigned int userId, char ***outMessages, int *messageCount);

#endif
