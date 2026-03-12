#include "proxy_client.h"
#include "tun_c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_SIZE 512
#define RESPONSE_SIZE 2048

static int read_line(const char* prompt, char* out, size_t out_size) {
    if (out == NULL || out_size == 0U) {
        return 0;
    }

    printf("%s", prompt);
    fflush(stdout);

    if (fgets(out, (int) out_size, stdin) == NULL) {
        return 0;
    }

    const size_t len = strlen(out);
    if (len > 0U && out[len - 1U] == '\n') {
        out[len - 1U] = '\0';
    }
    return 1;
}

int main(void) {
    char tun_if_name[64];
    char tun_ip[64];
    char proxy_host[128];
    char proxy_port_str[16];

    if (!read_line("TUN interface name (example: tun0): ", tun_if_name, sizeof(tun_if_name))) {
        return 1;
    }
    if (!read_line("TUN IPv4 CIDR (example: 10.20.0.1/24): ", tun_ip, sizeof(tun_ip))) {
        return 1;
    }
    if (!read_line("Proxy host (example: 127.0.0.1): ", proxy_host, sizeof(proxy_host))) {
        return 1;
    }
    if (!read_line("Proxy port (example: 8080): ", proxy_port_str, sizeof(proxy_port_str))) {
        return 1;
    }

    const unsigned long parsed_port = strtoul(proxy_port_str, NULL, 10);
    if (parsed_port == 0UL || parsed_port > 65535UL) {
        fprintf(stderr, "Invalid proxy port.\n");
        return 1;
    }

    tun_device* dev = tun_create(tun_if_name);
    if (dev == NULL) {
        fprintf(stderr, "Failed to create TUN device. Try running as root.\n");
    } else {
        if (!tun_set_ip(dev, tun_ip)) {
            fprintf(stderr, "Failed to set IP: %s\n", tun_last_error(dev));
        } else if (!tun_up(dev)) {
            fprintf(stderr, "Failed to bring interface up: %s\n", tun_last_error(dev));
        } else {
            printf("TUN device ready: %s\n", tun_name(dev));
        }
    }

    printf("\nSimple proxy UI ready. Type messages to send to proxy server.\n");
    printf("Type 'exit' to quit.\n\n");

    for (;;) {
        char request[INPUT_SIZE];
        char response[RESPONSE_SIZE];
        char err[256];

        if (!read_line("proxy> ", request, sizeof(request))) {
            break;
        }

        if (strcmp(request, "exit") == 0) {
            break;
        }

        const unsigned int sum = packet_checksum32((const unsigned char*) request, strlen(request));
        printf("request checksum32: %u\n", sum);

        if (proxy_send_request(proxy_host,
                               (unsigned short) parsed_port,
                               request,
                               response,
                               sizeof(response),
                               err,
                               sizeof(err)) != 0) {
            fprintf(stderr, "proxy error: %s\n", err);
            continue;
        }

        printf("proxy response:\n%s\n", response);
    }

    tun_close(dev);
    return 0;
}
