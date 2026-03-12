#include "tun_c.h"

#include "tun_device.hpp"

#include <new>

struct tun_device {
    tunlib::TunDevice* impl;
};

extern "C" {

tun_device* tun_create(const char* name) {
    tun_device* handle = new (std::nothrow) tun_device{};
    if (handle == nullptr) {
        return nullptr;
    }

    handle->impl = new (std::nothrow) tunlib::TunDevice();
    if (handle->impl == nullptr) {
        delete handle;
        return nullptr;
    }

    const char* safeName = (name == nullptr) ? "" : name;
    if (!handle->impl->openDevice(safeName)) {
        delete handle->impl;
        delete handle;
        return nullptr;
    }

    return handle;
}

int tun_set_ip(tun_device* dev, const char* ip_cidr) {
    if (dev == nullptr || dev->impl == nullptr || ip_cidr == nullptr) {
        return 0;
    }
    return dev->impl->setIPAddress(ip_cidr) ? 1 : 0;
}

int tun_up(tun_device* dev) {
    if (dev == nullptr || dev->impl == nullptr) {
        return 0;
    }
    return dev->impl->bringUp() ? 1 : 0;
}

int tun_read(tun_device* dev, uint8_t* buffer, size_t size) {
    if (dev == nullptr || dev->impl == nullptr) {
        return -1;
    }
    return dev->impl->readPacket(buffer, size);
}

int tun_write(tun_device* dev, const uint8_t* buffer, size_t size) {
    if (dev == nullptr || dev->impl == nullptr) {
        return -1;
    }
    return dev->impl->writePacket(buffer, size);
}

void tun_close(tun_device* dev) {
    if (dev == nullptr) {
        return;
    }
    delete dev->impl;
    delete dev;
}

const char* tun_name(const tun_device* dev) {
    if (dev == nullptr || dev->impl == nullptr) {
        return nullptr;
    }
    return dev->impl->name().c_str();
}

const char* tun_last_error(const tun_device* dev) {
    if (dev == nullptr || dev->impl == nullptr) {
        return nullptr;
    }
    return dev->impl->lastError().c_str();
}

} // extern "C"
