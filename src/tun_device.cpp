#include "tun_device.hpp"

#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <iostream>

namespace tunlib {

TunDevice::TunDevice()
{
    tun_fd = -1;
}

TunDevice::~TunDevice()
{
    if (tun_fd >= 0)
        close(tun_fd);
}

bool TunDevice::openDevice(const std::string& name)
{
    tun_fd = open("/dev/net/tun", O_RDWR);

    if (tun_fd < 0)
        return false;

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

    strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ);

    if (ioctl(tun_fd, TUNSETIFF, &ifr) < 0)
        return false;

    ifname = ifr.ifr_name;

    return true;
}

bool TunDevice::setIPAddress(const std::string& ip_cidr)
{
    std::string cmd = "ip addr add " + ip_cidr + " dev " + ifname;
    return execCmd(cmd);
}

bool TunDevice::bringUp()
{
    std::string cmd = "ip link set dev " + ifname + " up";
    return execCmd(cmd);
}

int TunDevice::readPacket(uint8_t* buffer, size_t size)
{
    return read(tun_fd, buffer, size);
}

int TunDevice::writePacket(const uint8_t* buffer, size_t size)
{
    return write(tun_fd, buffer, size);
}

int TunDevice::fd() const
{
    return tun_fd;
}

std::string TunDevice::name() const
{
    return ifname;
}

bool TunDevice::execCmd(const std::string& cmd)
{
    int r = system(cmd.c_str());
    return r == 0;
}

}