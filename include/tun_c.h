#ifndef TUNLIB_TUN_C_H
#define TUNLIB_TUN_C_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque C wrapper handle for TunDevice.
 */
typedef struct tun_device tun_device;

/**
 * @brief Allocate and open a TUN device.
 * @param name Requested interface name (e.g. "tun0"), may be empty string.
 * @return Handle on success, NULL on failure.
 */
tun_device* tun_create(const char* name);

/**
 * @brief Assign an IPv4 address to a TUN device.
 * @param dev Handle returned by tun_create.
 * @param ip_cidr IPv4 CIDR string (e.g. "10.8.0.1/24").
 * @return 1 on success, 0 on failure.
 */
int tun_set_ip(tun_device* dev, const char* ip_cidr);

/**
 * @brief Bring interface up.
 * @param dev Handle returned by tun_create.
 * @return 1 on success, 0 on failure.
 */
int tun_up(tun_device* dev);

/**
 * @brief Read one packet from TUN.
 * @param dev Handle returned by tun_create.
 * @param buffer Destination buffer.
 * @param size Buffer size in bytes.
 * @return Bytes read, or -1 on error.
 */
int tun_read(tun_device* dev, uint8_t* buffer, size_t size);

/**
 * @brief Write one packet to TUN.
 * @param dev Handle returned by tun_create.
 * @param buffer Source packet bytes.
 * @param size Packet length.
 * @return Bytes written, or -1 on error.
 */
int tun_write(tun_device* dev, const uint8_t* buffer, size_t size);

/**
 * @brief Close and free C handle.
 * @param dev Handle pointer; NULL is allowed.
 */
void tun_close(tun_device* dev);

/**
 * @brief Get device interface name.
 * @param dev Handle pointer.
 * @return Interface name owned by library, or NULL.
 */
const char* tun_name(const tun_device* dev);

/**
 * @brief Get last error string from wrapped C++ object.
 * @param dev Handle pointer.
 * @return Error string owned by library, or NULL.
 */
const char* tun_last_error(const tun_device* dev);

#ifdef __cplusplus
}
#endif

#endif // TUNLIB_TUN_C_H
