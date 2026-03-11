#include "tun_c.h"

#include <arpa/inet.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>

static volatile sig_atomic_t g_stop = 0;

static void handle_signal(int signo) {
    (void)signo;
    g_stop = 1;
}

static void print_packet_summary(const uint8_t* data, int len) {
    if (len < 20) {
        printf("short packet len=%d\n", len);
        return;
    }

    const uint8_t version = (uint8_t)((data[0] >> 4) & 0x0F);
    const uint8_t protocol = data[9];

    char src[INET_ADDRSTRLEN] = {0};
    char dst[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &data[12], src, sizeof(src));
    inet_ntop(AF_INET, &data[16], dst, sizeof(dst));

    const char* proto_name = "OTHER";
    if (protocol == 1) proto_name = "ICMP";
    if (protocol == 6) proto_name = "TCP";
    if (protocol == 17) proto_name = "UDP";

    printf("IPv%u %s -> %s proto=%s len=%d\n", (unsigned)version, src, dst, proto_name, len);
}

int main(void) {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    tun_device* tun = tun_create("tun1");
    if (tun == NULL) {
        fprintf(stderr, "tun_create failed\n");
        return 1;
    }

    if (!tun_set_ip(tun, "10.11.0.1/24")) {
        fprintf(stderr, "tun_set_ip failed: %s\n", tun_last_error(tun));
        tun_close(tun);
        return 1;
    }

    if (!tun_up(tun)) {
        fprintf(stderr, "tun_up failed: %s\n", tun_last_error(tun));
        tun_close(tun);
        return 1;
    }

    printf("TUN interface ready: %s (Ctrl+C to stop)\n", tun_name(tun));

    uint8_t buffer[2048] = {0};
    while (!g_stop) {
        const int n = tun_read(tun, buffer, sizeof(buffer));
        if (n > 0) {
            print_packet_summary(buffer, n);
            (void)tun_write(tun, buffer, (size_t)n);
        }
    }

    tun_close(tun);
    printf("Clean shutdown complete.\n");
    return 0;
}
