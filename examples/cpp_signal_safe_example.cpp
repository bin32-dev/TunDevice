#include "tun_device.hpp"

#include <arpa/inet.h>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace {
volatile std::sig_atomic_t gStop = 0;

void handleSignal(int) {
    gStop = 1;
}

void printPacketSummary(const uint8_t* data, int len) {
    if (len < 20) {
        std::cout << "short packet len=" << len << '\n';
        return;
    }

    const uint8_t version = (data[0] >> 4) & 0x0F;
    const uint8_t protocol = data[9];

    char src[INET_ADDRSTRLEN] = {0};
    char dst[INET_ADDRSTRLEN] = {0};
    ::inet_ntop(AF_INET, &data[12], src, sizeof(src));
    ::inet_ntop(AF_INET, &data[16], dst, sizeof(dst));

    const char* protoName = "OTHER";
    if (protocol == 1) protoName = "ICMP";
    if (protocol == 6) protoName = "TCP";
    if (protocol == 17) protoName = "UDP";

    std::cout << "IPv" << static_cast<int>(version) << " " << src << " -> " << dst
              << " proto=" << protoName << " len=" << len << '\n';
}
} // namespace

int main() {
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    tunlib::TunDevice tun;
    if (!tun.openDevice("tun0")) {
        std::cerr << "openDevice failed: " << tun.lastError() << '\n';
        return 1;
    }

    if (!tun.setIPAddress("10.10.0.1/24")) {
        std::cerr << "setIPAddress failed: " << tun.lastError() << '\n';
        return 1;
    }

    if (!tun.bringUp()) {
        std::cerr << "bringUp failed: " << tun.lastError() << '\n';
        return 1;
    }

    std::cout << "TUN interface ready: " << tun.name() << " (Ctrl+C to stop)" << '\n';

    uint8_t buffer[2048] = {0};
    while (!gStop) {
        const int n = tun.readPacket(buffer, sizeof(buffer));
        if (n > 0) {
            printPacketSummary(buffer, n);
            const int written = tun.writePacket(buffer, static_cast<std::size_t>(n));
            if (written < 0) {
                std::cerr << "writePacket failed" << '\n';
            }
        }
    }

    tun.closeDevice();
    std::cout << "Clean shutdown complete." << '\n';
    return 0;
}
