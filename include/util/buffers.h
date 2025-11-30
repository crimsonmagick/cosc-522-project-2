/**
*  Interface for buffer functions, managing the access of bytes of a buffer in a network-safe way, converting from little-endian
 * to big-endian and from big-endian to little-endian as needed.
 */

#ifndef COSC522_LODI_BUFFERS_H
#define COSC522_LODI_BUFFERS_H

void appendUint8(char *buffer, size_t *offset, char value);

void appendUint32(char *buffer, size_t *offset, uint32_t value);

void appendUint64(char *buffer, size_t *offset, uint64_t value);

char getUint8(const char *buffer, size_t *offset);

uint32_t getUint32(const char *buffer, size_t *offset);

uint64_t getUint64(const char *buffer, size_t *offset);

#endif //COSC522_LODI_BUFFERS_H
