#ifndef TUN_PROXY_CLIENT_H
#define TUN_PROXY_CLIENT_H

#include <stddef.h>
#include <stdint.h>

typedef enum proxy_protocol {
    PROXY_PROTOCOL_BINARY = 0,
    PROXY_PROTOCOL_HTTP = 1
} proxy_protocol;

typedef struct proxy_connection {
    int sock;
    proxy_protocol protocol;
    char host[128];
    unsigned short port;
    char http_path[128];
} proxy_connection;

int proxy_open_connection(const char* host,
                          unsigned short port,
                          proxy_protocol protocol,
                          const char* http_path,
                          proxy_connection* out_conn,
                          char* error,
                          size_t error_size);

int proxy_forward_packet(proxy_connection* conn,
                         const uint8_t* packet,
                         size_t packet_size,
                         uint8_t* response,
                         size_t response_capacity,
                         size_t* response_size,
                         char* error,
                         size_t error_size);

void proxy_close_connection(proxy_connection* conn);

unsigned int packet_checksum32(const unsigned char* data, size_t len);

#endif
