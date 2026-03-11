#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace tunlib {

class TunDevice
{
public:
    TunDevice();
    ~TunDevice();

    bool openDevice(const std::string& name);

    bool setIPAddress(const std::string& ip_cidr);
    bool bringUp();

    int readPacket(uint8_t* buffer, size_t size);
    int writePacket(const uint8_t* buffer, size_t size);

    int fd() const;
    std::string name() const;

private:
    int tun_fd;
    std::string ifname;

    bool execCmd(const std::string& cmd);
};

}