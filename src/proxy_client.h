#ifndef TUN_PROXY_CLIENT_H
#define TUN_PROXY_CLIENT_H

#include <stddef.h>

int proxy_send_request(const char* host,
                       unsigned short port,
                       const char* request,
                       char* response,
                       size_t response_size,
                       char* error,
                       size_t error_size);

unsigned int packet_checksum32(const unsigned char* data, size_t len);

#endif
