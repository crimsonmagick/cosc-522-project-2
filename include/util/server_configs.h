/**
*  Shared interface for server config helper functions, retrieving server IP Addresses and Ports.
*  The functions enable sensible defaults if the addresses and ports are not configured as environment variables.
 */

#ifndef COSC522_LODI_SERVER_CONFIGS_H
#define COSC522_LODI_SERVER_CONFIGS_H
#include <netinet/in.h>

enum Server {
  PK,
  LODI,
  TFA
};

typedef struct {
  char *address;
  char *port;
} ServerConfig;

ServerConfig getServerConfig(const enum Server server);

#endif