#define _POSIX_C_SOURCE 200809L
#include "proxy_client.h"
#include "tun_c.h"

#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PACKET_SIZE 4096

static volatile sig_atomic_t g_stop = 0;

static void on_signal(int signum) {
    (void) signum;
    g_stop = 1;
}

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

static void live_log(const char* tag, const char* fmt, ...) {
    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);

    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm_now);

    printf("[%s] [%s] ", ts, tag);

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    fflush(stdout);
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
        return 1;
    }

    if (!tun_set_ip(dev, tun_ip)) {
        fprintf(stderr, "Failed to set IP: %s\n", tun_last_error(dev));
        tun_close(dev);
        return 1;
    }

    if (!tun_up(dev)) {
        fprintf(stderr, "Failed to bring interface up: %s\n", tun_last_error(dev));
        tun_close(dev);
        return 1;
    }

    live_log("INFO", "TUN device ready: %s (%s)", tun_name(dev), tun_ip);
    live_log("INFO", "Routing reminder: Linux will not use %s until you add route rules.", tun_name(dev));
    live_log("INFO", "Example: ip route add 0.0.0.0/1 dev %s && ip route add 128.0.0.0/1 dev %s", tun_name(dev), tun_name(dev));

    int proxy_sock = -1;
    char err[256];
    if (proxy_open_connection(proxy_host, (unsigned short) parsed_port, &proxy_sock, err, sizeof(err)) != 0) {
        fprintf(stderr, "Failed to connect proxy server: %s\n", err);
        tun_close(dev);
        return 1;
    }

    live_log("INFO", "Connected to proxy server %s:%lu", proxy_host, parsed_port);
    live_log("INFO", "Packet forwarding loop is running. Press Ctrl+C to stop.");

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    uint8_t packet[PACKET_SIZE];
    uint8_t response[PACKET_SIZE];

    while (!g_stop) {
        const int read_rc = tun_read(dev, packet, sizeof(packet));
        if (read_rc <= 0) {
            live_log("ERROR", "tun_read failed: %s", tun_last_error(dev));
            break;
        }

        size_t response_size = 0U;
        const unsigned int checksum = packet_checksum32(packet, (size_t) read_rc);
        live_log("TUN->PROXY", "captured packet bytes=%d checksum32=%u", read_rc, checksum);

        if (proxy_forward_packet(proxy_sock,
                                 packet,
                                 (size_t) read_rc,
                                 response,
                                 sizeof(response),
                                 &response_size,
                                 err,
                                 sizeof(err)) != 0) {
            live_log("ERROR", "proxy forward failed: %s", err);
            break;
        }

        live_log("PROXY->TUN", "received packet bytes=%zu", response_size);

        const int write_rc = tun_write(dev, response, response_size);
        if (write_rc <= 0) {
            live_log("ERROR", "tun_write failed: %s", tun_last_error(dev));
            break;
        }

        live_log("INFO", "packet forwarded successfully (%d bytes written)", write_rc);
    }

    proxy_close_connection(&proxy_sock);
    tun_close(dev);
    live_log("INFO", "Tunnel session closed");
    return 0;
}
