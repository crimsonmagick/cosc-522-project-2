/**
 * Manages the lifecycle of a "listener" i.e. a logged-in user streaming idol messages in real time.
 */

#ifndef COSC522_LODI_LISTENER_REPOSITORY_H
#define COSC522_LODI_LISTENER_REPOSITORY_H
#include "collections/list.h"
#include "domain/domain.h"

void initListenerRepository();
int addListener(ClientHandle *listener);
int removeListener(ClientHandle *listener);
int getAllListeners(List **listenersOut);

#endif
