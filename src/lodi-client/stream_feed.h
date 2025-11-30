#ifndef COSC522_LODI_PRINT_FEED_H
#define COSC522_LODI_PRINT_FEED_H

int startStreamFeed(unsigned int userId, unsigned long timestamp, unsigned long digitalSignature);

void stopStreamFeed(int pid);

#endif
