#include "proxy_client.h"

unsigned int packet_checksum32(const unsigned char* data, size_t len) {
    unsigned int acc = 0U;
    if (data == 0) {
        return 0U;
    }

    for (size_t i = 0; i < len; ++i) {
        acc += data[i];
    }
    return acc;
}
