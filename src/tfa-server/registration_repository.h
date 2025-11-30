/**
* Provides persistence interface for registered client ip addresses and ports
 */

#ifndef COSC522_LODI_KEY_REPOSITORY_H
#define COSC522_LODI_KEY_REPOSITORY_H
#include "domain/domain.h"

int registerClient(unsigned int userId, const ClientHandle *clientHandleIn);

int getClient(unsigned int userId, ClientHandle **clientHandleOut);

#endif
