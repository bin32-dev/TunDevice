#ifndef TUN_PROXY_CLIENT_H
#define TUN_PROXY_CLIENT_H

#include <stddef.h>
#include <stdint.h>

int proxy_open_connection(const char* host,
                          unsigned short port,
                          int* out_sock,
                          char* error,
                          size_t error_size);

int proxy_forward_packet(int sock,
                         const uint8_t* packet,
                         size_t packet_size,
                         uint8_t* response,
                         size_t response_capacity,
                         size_t* response_size,
                         char* error,
                         size_t error_size);

void proxy_close_connection(int* sock);

unsigned int packet_checksum32(const unsigned char* data, size_t len);

#endif
