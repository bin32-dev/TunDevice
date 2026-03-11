# TunDevice (Linux TUN Library)

TunDevice is a production-ready Linux TUN device library with both C++ and C APIs.
It supports:

- TUN device creation and management.
- IPv4 address assignment (CIDR format).
- Interface activation (`UP` state).
- Reading/writing raw IPv4 packets (TCP/UDP/ICMP carried in IP packets).
- Signal-safe shutdown in sample applications.

> Note: Creating/configuring TUN interfaces generally requires root privileges or equivalent Linux capabilities.

## Project Layout

- `include/` Public headers (`tun_device.hpp`, `tun_c.h`)
- `src/` Library implementation
- `examples/` C and C++ examples with Ctrl+C-safe loop termination
- `docs/` Doxygen-generated HTML output
- `build/` Out-of-tree CMake build directory
- `lib/` Shared library output after build

## Build

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

Outputs:
- `libtunlib.so` shared library
- `tun_cpp_example` and `tun_c_example` example binaries

## Install / Uninstall

```bash
cd build
sudo cmake --install .
```

To uninstall:

```bash
cd build
sudo cmake --build . --target uninstall
```

## Generate Documentation (Doxygen HTML)

```bash
cd build
cmake --build . --target doc
```

Generated HTML docs are written to:

- `docs/html/index.html`

## C++ API Quick Example

```cpp
#include "tun_device.hpp"

tunlib::TunDevice tun;
if (!tun.openDevice("tun0")) return 1;
if (!tun.setIPAddress("10.10.0.1/24")) return 1;
if (!tun.bringUp()) return 1;
```

## C API Quick Example

```c
#include "tun_c.h"

tun_device* tun = tun_create("tun1");
if (!tun) return 1;
if (!tun_set_ip(tun, "10.11.0.1/24")) return 1;
if (!tun_up(tun)) return 1;
tun_close(tun);
```

## Run Examples

```bash
sudo ./build/tun_cpp_example
sudo ./build/tun_c_example
```

Then generate traffic to the assigned subnet and observe packet summaries.
