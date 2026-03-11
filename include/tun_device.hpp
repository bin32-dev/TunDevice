#ifndef TUNLIB_TUN_DEVICE_HPP
#define TUNLIB_TUN_DEVICE_HPP

#include <cstddef>
#include <cstdint>
#include <string>

namespace tunlib {

/**
 * @brief Linux TUN (layer-3) virtual network interface wrapper.
 *
 * The TunDevice class provides safe RAII-based management of a TUN device file
 * descriptor and helper methods for assigning an IPv4 address, bringing the
 * interface up, and reading/writing raw IPv4 packets (TCP/UDP/ICMP payloads
 * encapsulated in IP datagrams).
 */
class TunDevice {
public:
    /**
     * @brief Construct an unopened TUN device object.
     */
    TunDevice();

    /**
     * @brief Non-copyable (file-descriptor ownership).
     */
    TunDevice(const TunDevice&) = delete;

    /**
     * @brief Non-copyable (file-descriptor ownership).
     */
    TunDevice& operator=(const TunDevice&) = delete;

    /**
     * @brief Move constructor.
     */
    TunDevice(TunDevice&& other) noexcept;

    /**
     * @brief Move assignment.
     */
    TunDevice& operator=(TunDevice&& other) noexcept;

    /**
     * @brief Destructor closes the device if still open.
     */
    ~TunDevice();

    /**
     * @brief Open and create/attach a Linux TUN device.
     * @param requestedName Device name (e.g. "tun0"). Empty string lets kernel choose.
     * @return true on success, false on failure.
     */
    bool openDevice(const std::string& requestedName);

    /**
     * @brief Assign IPv4 address to interface.
     * @param ipCidr CIDR string (e.g. "10.8.0.1/24").
     * @return true on success, false on failure.
     */
    bool setIPAddress(const std::string& ipCidr);

    /**
     * @brief Bring the interface administratively up.
     * @return true on success, false on failure.
     */
    bool bringUp();

    /**
     * @brief Read one packet from TUN fd.
     * @param buffer Destination buffer.
     * @param size Buffer size in bytes.
     * @return Number of bytes read, or -1 on error.
     */
    int readPacket(uint8_t* buffer, std::size_t size) const;

    /**
     * @brief Write one packet to TUN fd.
     * @param buffer Source buffer.
     * @param size Number of bytes to write.
     * @return Number of bytes written, or -1 on error.
     */
    int writePacket(const uint8_t* buffer, std::size_t size) const;

    /**
     * @brief Close the device if open.
     */
    void closeDevice() noexcept;

    /**
     * @brief Get the interface name assigned by kernel.
     */
    const std::string& name() const noexcept;

    /**
     * @brief Get underlying file descriptor.
     */
    int fd() const noexcept;

    /**
     * @brief Last human-readable error description from API call.
     */
    const std::string& lastError() const noexcept;

private:
    bool configureInterfaceAddress(const std::string& ip, const std::string& mask);
    bool parseIPv4Cidr(const std::string& ipCidr, std::string& ipOut, std::string& maskOut);
    void setError(const std::string& message);

    int tunFd_;
    std::string ifName_;
    std::string lastError_;
};

} // namespace tunlib

#endif // TUNLIB_TUN_DEVICE_HPP
