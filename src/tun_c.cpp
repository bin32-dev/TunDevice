#include "tun_device.hpp"
#include "tun_c.h"

using namespace tunlib;

struct tun_device
{
    TunDevice* dev;
};

tun_device* tun_create(const char* name)
{
    tun_device* t = new tun_device;

    t->dev = new TunDevice();

    if (!t->dev->openDevice(name))
    {
        delete t->dev;
        delete t;
        return nullptr;
    }

    return t;
}

int tun_set_ip(tun_device* t, const char* ip)
{
    return t->dev->setIPAddress(ip);
}

int tun_up(tun_device* t)
{
    return t->dev->bringUp();
}

int tun_read(tun_device* t, uint8_t* buf, size_t size)
{
    return t->dev->readPacket(buf, size);
}

int tun_write(tun_device* t, const uint8_t* buf, size_t size)
{
    return t->dev->writePacket(buf, size);
}

void tun_close(tun_device* t)
{
    delete t->dev;
    delete t;
}