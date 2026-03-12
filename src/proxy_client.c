#define _POSIX_C_SOURCE 200112L
#include "proxy_client.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void write_error(char* error, size_t error_size, const char* message) {
    if (error != NULL && error_size > 0U) {
        snprintf(error, error_size, "%s", message);
    }
}

static int send_all(int sock, const uint8_t* buf, size_t len, char* error, size_t error_size) {
    size_t sent = 0U;
    while (sent < len) {
        const ssize_t rc = send(sock, buf + sent, len - sent, 0);
        if (rc <= 0) {
            write_error(error, error_size, strerror(errno));
            return -1;
        }
        sent += (size_t) rc;
    }
    return 0;
}

static int recv_all(int sock, uint8_t* buf, size_t len, char* error, size_t error_size) {
    size_t got = 0U;
    while (got < len) {
        const ssize_t rc = recv(sock, buf + got, len - got, 0);
        if (rc <= 0) {
            if (rc == 0) {
                write_error(error, error_size, "connection closed by proxy");
            } else {
                write_error(error, error_size, strerror(errno));
            }
            return -1;
        }
        got += (size_t) rc;
    }
    return 0;
}

int proxy_open_connection(const char* host,
                          unsigned short port,
                          int* out_sock,
                          char* error,
                          size_t error_size) {
    if (host == NULL || out_sock == NULL) {
        write_error(error, error_size, "invalid argument");
        return -1;
    }

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%hu", port);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* result = NULL;
    const int gai_rc = getaddrinfo(host, port_str, &hints, &result);
    if (gai_rc != 0) {
        write_error(error, error_size, gai_strerror(gai_rc));
        return -1;
    }

    int sock = -1;
    for (struct addrinfo* it = result; it != NULL; it = it->ai_next) {
        sock = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (sock < 0) {
            continue;
        }

        if (connect(sock, it->ai_addr, it->ai_addrlen) == 0) {
            break;
        }

        close(sock);
        sock = -1;
    }

    freeaddrinfo(result);

    if (sock < 0) {
        write_error(error, error_size, strerror(errno));
        return -1;
    }

    *out_sock = sock;
    write_error(error, error_size, "");
    return 0;
}

int proxy_forward_packet(int sock,
                         const uint8_t* packet,
                         size_t packet_size,
                         uint8_t* response,
                         size_t response_capacity,
                         size_t* response_size,
                         char* error,
                         size_t error_size) {
    if (sock < 0 || packet == NULL || response == NULL || response_size == NULL || packet_size > UINT32_MAX) {
        write_error(error, error_size, "invalid argument");
        return -1;
    }

    const uint32_t req_len_be = htonl((uint32_t) packet_size);
    if (send_all(sock, (const uint8_t*) &req_len_be, sizeof(req_len_be), error, error_size) != 0) {
        return -1;
    }

    if (send_all(sock, packet, packet_size, error, error_size) != 0) {
        return -1;
    }

    uint32_t resp_len_be = 0U;
    if (recv_all(sock, (uint8_t*) &resp_len_be, sizeof(resp_len_be), error, error_size) != 0) {
        return -1;
    }

    const size_t resp_len = (size_t) ntohl(resp_len_be);
    if (resp_len > response_capacity) {
        write_error(error, error_size, "proxy response packet too large");
        return -1;
    }

    if (recv_all(sock, response, resp_len, error, error_size) != 0) {
        return -1;
    }

    *response_size = resp_len;
    write_error(error, error_size, "");
    return 0;
}

void proxy_close_connection(int* sock) {
    if (sock != NULL && *sock >= 0) {
        close(*sock);
        *sock = -1;
    }
}
