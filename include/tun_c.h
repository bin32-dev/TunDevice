#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tun_device tun_device;

tun_device* tun_create(const char* name);

int tun_set_ip(tun_device* dev, const char* ip);

int tun_up(tun_device* dev);

int tun_read(tun_device* dev, uint8_t* buffer, size_t size);

int tun_write(tun_device* dev, const uint8_t* buffer, size_t size);

void tun_close(tun_device* dev);

#ifdef __cplusplus
}
#endif