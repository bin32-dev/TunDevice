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

int proxy_send_request(const char* host,
                       unsigned short port,
                       const char* request,
                       char* response,
                       size_t response_size,
                       char* error,
                       size_t error_size) {
    if (host == NULL || request == NULL || response == NULL || response_size == 0U) {
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

    const size_t req_len = strlen(request);
    if (send(sock, request, req_len, 0) < 0) {
        write_error(error, error_size, strerror(errno));
        close(sock);
        return -1;
    }

    if (send(sock, "\n", 1, 0) < 0) {
        write_error(error, error_size, strerror(errno));
        close(sock);
        return -1;
    }

    const ssize_t rc = recv(sock, response, response_size - 1U, 0);
    if (rc < 0) {
        write_error(error, error_size, strerror(errno));
        close(sock);
        return -1;
    }

    response[(size_t) rc] = '\0';
    close(sock);
    write_error(error, error_size, "");
    return 0;
}
