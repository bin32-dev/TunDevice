# TunDevice Workspace

This workspace builds:

- `libtunlib.so` (C/C++ Linux TUN library)
- `tun_proxy_ui` (C terminal UI that uses the library and forwards user requests to a TCP proxy server)

## Quick build

```bash
./scripts/build.sh
```

Clean CMake cache and rebuild:

```bash
./scripts/build.sh --clean
```

All outputs are generated under `build/`:

- `build/bin/tun_proxy_ui`
- `build/lib/libtunlib.so`

## Run

```bash
sudo ./build/bin/tun_proxy_ui
```

> Creating/configuring TUN devices usually requires root privileges.
