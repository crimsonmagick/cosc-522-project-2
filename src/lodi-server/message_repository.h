/**
* Provides persistence interface for registered client ip addresses and ports
*/

#ifndef COSC522_LODI_KEY_REPOSITORY_H
#define COSC522_LODI_KEY_REPOSITORY_H
#include "collections/list.h"

int addMessage(unsigned int userId, char *message);

int getMessages(unsigned int userId, List **outMessages);

#endif
