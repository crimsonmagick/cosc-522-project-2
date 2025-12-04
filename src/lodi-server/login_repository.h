/**
 * Manages the lifecycle of a logged-in user.
 */

#ifndef COSC522_LODI_LOGIN_REPOSITORY_H
#define COSC522_LODI_LOGIN_REPOSITORY_H
#include "domain/domain.h"

int userLogin(const ClientHandle *userClient);

int userLogout(const ClientHandle *userClient);

int isUserLoggedIn(const ClientHandle *userClient);
#endif
