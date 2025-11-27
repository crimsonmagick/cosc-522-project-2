#ifndef COSC522_LODI_LISTENER_REPOSITORY_H
#define COSC522_LODI_LISTENER_REPOSITORY_H
#include "collections/list.h"
#include "domain/domain.h"

void initListenerRepository();
int addListener(DomainHandle *listener);
int removeListener(DomainHandle *listener);
int getAllListeners(List **listenersOut);

#endif
