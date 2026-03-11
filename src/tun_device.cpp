#include "tun_device.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sstream>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

namespace tunlib {

namespace {
constexpr std::size_t kMaxPrefixLength = 32;

in_addr prefixToMask(unsigned int prefix) {
    in_addr mask{};
    if (prefix == 0) {
        mask.s_addr = 0;
        return mask;
    }
    const uint32_t maskHostOrder = (~uint32_t{0}) << (kMaxPrefixLength - prefix);
    mask.s_addr = htonl(maskHostOrder);
    return mask;
}

std::string errnoMessage(const std::string& prefix) {
    std::ostringstream oss;
    oss << prefix << ": " << std::strerror(errno);
    return oss.str();
}
} // namespace

TunDevice::TunDevice() : tunFd_(-1) {}

TunDevice::TunDevice(TunDevice&& other) noexcept
    : tunFd_(other.tunFd_), ifName_(std::move(other.ifName_)), lastError_(std::move(other.lastError_)) {
    other.tunFd_ = -1;
}

TunDevice& TunDevice::operator=(TunDevice&& other) noexcept {
    if (this != &other) {
        closeDevice();
        tunFd_ = other.tunFd_;
        ifName_ = std::move(other.ifName_);
        lastError_ = std::move(other.lastError_);
        other.tunFd_ = -1;
    }
    return *this;
}

TunDevice::~TunDevice() {
    closeDevice();
}

bool TunDevice::openDevice(const std::string& requestedName) {
    closeDevice();

    tunFd_ = ::open("/dev/net/tun", O_RDWR);
    if (tunFd_ < 0) {
        setError(errnoMessage("open(/dev/net/tun) failed"));
        return false;
    }

    ifreq ifr{};
    ifr.ifr_flags = static_cast<short>(IFF_TUN | IFF_NO_PI);

    if (!requestedName.empty()) {
        std::strncpy(ifr.ifr_name, requestedName.c_str(), IFNAMSIZ - 1);
        ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    }

    if (::ioctl(tunFd_, TUNSETIFF, &ifr) < 0) {
        setError(errnoMessage("ioctl(TUNSETIFF) failed"));
        closeDevice();
        return false;
    }

    ifName_ = ifr.ifr_name;
    lastError_.clear();
    return true;
}

bool TunDevice::setIPAddress(const std::string& ipCidr) {
    std::string ip;
    std::string mask;
    if (!parseIPv4Cidr(ipCidr, ip, mask)) {
        return false;
    }
    return configureInterfaceAddress(ip, mask);
}

bool TunDevice::bringUp() {
    if (ifName_.empty()) {
        setError("bringUp failed: device not opened");
        return false;
    }

    const int sockFd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sockFd < 0) {
        setError(errnoMessage("socket(AF_INET, SOCK_DGRAM) failed"));
        return false;
    }

    ifreq ifr{};
    std::strncpy(ifr.ifr_name, ifName_.c_str(), IFNAMSIZ - 1);

    if (::ioctl(sockFd, SIOCGIFFLAGS, &ifr) < 0) {
        setError(errnoMessage("ioctl(SIOCGIFFLAGS) failed"));
        ::close(sockFd);
        return false;
    }

    ifr.ifr_flags = static_cast<short>(ifr.ifr_flags | IFF_UP | IFF_RUNNING);

    if (::ioctl(sockFd, SIOCSIFFLAGS, &ifr) < 0) {
        setError(errnoMessage("ioctl(SIOCSIFFLAGS) failed"));
        ::close(sockFd);
        return false;
    }

    ::close(sockFd);
    lastError_.clear();
    return true;
}

int TunDevice::readPacket(uint8_t* buffer, std::size_t size) const {
    if (tunFd_ < 0 || buffer == nullptr || size == 0) {
        return -1;
    }
    const ssize_t rc = ::read(tunFd_, buffer, size);
    return static_cast<int>(rc);
}

int TunDevice::writePacket(const uint8_t* buffer, std::size_t size) const {
    if (tunFd_ < 0 || buffer == nullptr || size == 0) {
        return -1;
    }
    const ssize_t rc = ::write(tunFd_, buffer, size);
    return static_cast<int>(rc);
}

void TunDevice::closeDevice() noexcept {
    if (tunFd_ >= 0) {
        ::close(tunFd_);
        tunFd_ = -1;
    }
}

const std::string& TunDevice::name() const noexcept {
    return ifName_;
}

int TunDevice::fd() const noexcept {
    return tunFd_;
}

const std::string& TunDevice::lastError() const noexcept {
    return lastError_;
}

bool TunDevice::configureInterfaceAddress(const std::string& ip, const std::string& mask) {
    if (ifName_.empty()) {
        setError("setIPAddress failed: device not opened");
        return false;
    }

    const int sockFd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sockFd < 0) {
        setError(errnoMessage("socket(AF_INET, SOCK_DGRAM) failed"));
        return false;
    }

    ifreq ifr{};
    std::strncpy(ifr.ifr_name, ifName_.c_str(), IFNAMSIZ - 1);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;

    if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        setError("Invalid IPv4 address: " + ip);
        ::close(sockFd);
        return false;
    }

    std::memcpy(&ifr.ifr_addr, &addr, sizeof(sockaddr_in));
    if (::ioctl(sockFd, SIOCSIFADDR, &ifr) < 0) {
        setError(errnoMessage("ioctl(SIOCSIFADDR) failed"));
        ::close(sockFd);
        return false;
    }

    sockaddr_in netmaskAddr{};
    netmaskAddr.sin_family = AF_INET;
    if (::inet_pton(AF_INET, mask.c_str(), &netmaskAddr.sin_addr) != 1) {
        setError("Invalid netmask: " + mask);
        ::close(sockFd);
        return false;
    }

    std::memcpy(&ifr.ifr_netmask, &netmaskAddr, sizeof(sockaddr_in));
    if (::ioctl(sockFd, SIOCSIFNETMASK, &ifr) < 0) {
        setError(errnoMessage("ioctl(SIOCSIFNETMASK) failed"));
        ::close(sockFd);
        return false;
    }

    ::close(sockFd);
    lastError_.clear();
    return true;
}

bool TunDevice::parseIPv4Cidr(const std::string& ipCidr, std::string& ipOut, std::string& maskOut) {
    const auto slashPos = ipCidr.find('/');
    if (slashPos == std::string::npos) {
        setError("CIDR must be in form a.b.c.d/prefix");
        return false;
    }

    ipOut = ipCidr.substr(0, slashPos);
    const std::string prefixText = ipCidr.substr(slashPos + 1);

    char* end = nullptr;
    errno = 0;
    const unsigned long prefix = std::strtoul(prefixText.c_str(), &end, 10);
    if (errno != 0 || end == prefixText.c_str() || *end != '\0' || prefix > kMaxPrefixLength) {
        setError("Invalid CIDR prefix length: " + prefixText);
        return false;
    }

    in_addr ipAddr{};
    if (::inet_pton(AF_INET, ipOut.c_str(), &ipAddr) != 1) {
        setError("Invalid IPv4 address: " + ipOut);
        return false;
    }

    char maskBuffer[INET_ADDRSTRLEN] = {0};
    const in_addr maskAddr = prefixToMask(static_cast<unsigned int>(prefix));
    if (::inet_ntop(AF_INET, &maskAddr, maskBuffer, sizeof(maskBuffer)) == nullptr) {
        setError(errnoMessage("inet_ntop failed for netmask"));
        return false;
    }

    maskOut = maskBuffer;
    lastError_.clear();
    return true;
}

void TunDevice::setError(const std::string& message) {
    lastError_ = message;
}

} // namespace tunlib
