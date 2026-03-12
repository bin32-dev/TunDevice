# LibTUN

`LibTUN` provides a Linux TUN device library with both C++ and C APIs.

- Public headers: `include/tun_device.hpp`, `include/tun_c.h`
- Sources: `src/tun_device.cpp`, `src/tun_c.cpp`

This library is built by the workspace top-level `CMakeLists.txt`.

## Standalone build

```bash
cmake -S . -B build
cmake --build build
```

The workspace config places binaries/libraries into the top-level `build/bin` and `build/lib` directories.
