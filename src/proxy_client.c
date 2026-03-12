#define _POSIX_C_SOURCE 200112L
#include "proxy_client.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
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

static int recv_line(int sock, char* line, size_t line_cap, char* error, size_t error_size) {
    if (line == NULL || line_cap == 0U) {
        write_error(error, error_size, "invalid line buffer");
        return -1;
    }

    size_t idx = 0U;
    while (idx + 1U < line_cap) {
        char ch;
        const ssize_t rc = recv(sock, &ch, 1, 0);
        if (rc <= 0) {
            if (rc == 0) {
                write_error(error, error_size, "connection closed by proxy");
            } else {
                write_error(error, error_size, strerror(errno));
            }
            return -1;
        }
        line[idx++] = ch;
        if (idx >= 2U && line[idx - 2U] == '\r' && line[idx - 1U] == '\n') {
            line[idx] = '\0';
            return 0;
        }
    }

    write_error(error, error_size, "HTTP header line too long");
    return -1;
}

static int parse_content_length(const char* line, size_t* out_len) {
    const char* key = "Content-Length:";
    const size_t key_len = strlen(key);
    if (strncasecmp(line, key, key_len) != 0) {
        return 0;
    }

    const char* it = line + key_len;
    while (*it != '\0' && isspace((unsigned char) *it)) {
        ++it;
    }

    char* end_ptr = NULL;
    const unsigned long long value = strtoull(it, &end_ptr, 10);
    if (end_ptr == it) {
        return -1;
    }

    *out_len = (size_t) value;
    return 1;
}

int proxy_open_connection(const char* host,
                          unsigned short port,
                          proxy_protocol protocol,
                          const char* http_path,
                          proxy_connection* out_conn,
                          char* error,
                          size_t error_size) {
    if (host == NULL || out_conn == NULL) {
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

    memset(out_conn, 0, sizeof(*out_conn));
    out_conn->sock = sock;
    out_conn->protocol = protocol;
    out_conn->port = port;
    snprintf(out_conn->host, sizeof(out_conn->host), "%s", host);
    snprintf(out_conn->http_path, sizeof(out_conn->http_path), "%s", (http_path != NULL && http_path[0] != '\0') ? http_path : "/");
    write_error(error, error_size, "");
    return 0;
}

static int proxy_forward_binary(int sock,
                                const uint8_t* packet,
                                size_t packet_size,
                                uint8_t* response,
                                size_t response_capacity,
                                size_t* response_size,
                                char* error,
                                size_t error_size) {
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
    return 0;
}

static int proxy_forward_http(proxy_connection* conn,
                              const uint8_t* packet,
                              size_t packet_size,
                              uint8_t* response,
                              size_t response_capacity,
                              size_t* response_size,
                              char* error,
                              size_t error_size) {
    char req_header[512];
    const int header_len = snprintf(req_header,
                                    sizeof(req_header),
                                    "POST %s HTTP/1.1\r\n"
                                    "Host: %s:%hu\r\n"
                                    "Content-Type: application/octet-stream\r\n"
                                    "Content-Length: %zu\r\n"
                                    "Connection: keep-alive\r\n"
                                    "\r\n",
                                    conn->http_path,
                                    conn->host,
                                    conn->port,
                                    packet_size);
    if (header_len <= 0 || (size_t) header_len >= sizeof(req_header)) {
        write_error(error, error_size, "HTTP request header too long");
        return -1;
    }

    if (send_all(conn->sock, (const uint8_t*) req_header, (size_t) header_len, error, error_size) != 0) {
        return -1;
    }
    if (send_all(conn->sock, packet, packet_size, error, error_size) != 0) {
        return -1;
    }

    char line[512];
    if (recv_line(conn->sock, line, sizeof(line), error, error_size) != 0) {
        return -1;
    }

    int status_code = 0;
    if (sscanf(line, "HTTP/%*d.%*d %d", &status_code) != 1 || status_code < 200 || status_code > 299) {
        write_error(error, error_size, "HTTP proxy returned non-success status");
        return -1;
    }

    size_t content_length = 0U;
    int have_content_length = 0;
    for (;;) {
        if (recv_line(conn->sock, line, sizeof(line), error, error_size) != 0) {
            return -1;
        }
        if (strcmp(line, "\r\n") == 0) {
            break;
        }

        size_t parsed_len = 0U;
        const int rc = parse_content_length(line, &parsed_len);
        if (rc < 0) {
            write_error(error, error_size, "invalid Content-Length in HTTP response");
            return -1;
        }
        if (rc > 0) {
            content_length = parsed_len;
            have_content_length = 1;
        }
    }

    if (!have_content_length) {
        write_error(error, error_size, "HTTP response missing Content-Length");
        return -1;
    }
    if (content_length > response_capacity) {
        write_error(error, error_size, "HTTP response body too large");
        return -1;
    }

    if (recv_all(conn->sock, response, content_length, error, error_size) != 0) {
        return -1;
    }

    *response_size = content_length;
    return 0;
}

int proxy_forward_packet(proxy_connection* conn,
                         const uint8_t* packet,
                         size_t packet_size,
                         uint8_t* response,
                         size_t response_capacity,
                         size_t* response_size,
                         char* error,
                         size_t error_size) {
    if (conn == NULL || conn->sock < 0 || packet == NULL || response == NULL || response_size == NULL ||
        packet_size > UINT32_MAX) {
        write_error(error, error_size, "invalid argument");
        return -1;
    }

    if (conn->protocol == PROXY_PROTOCOL_HTTP) {
        return proxy_forward_http(conn, packet, packet_size, response, response_capacity, response_size, error, error_size);
    }

    return proxy_forward_binary(conn->sock, packet, packet_size, response, response_capacity, response_size, error, error_size);
}

void proxy_close_connection(proxy_connection* conn) {
    if (conn != NULL && conn->sock >= 0) {
        close(conn->sock);
        conn->sock = -1;
    }
}
