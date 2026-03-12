# TunDevice Workspace

This workspace builds:

- `libtunlib.so` (C/C++ Linux TUN library)
- `tun_proxy_ui` (C terminal app that creates a TUN device and forwards packets to a proxy server)

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

## Forwarding flow (implemented)

The app now runs the real forwarding loop:

1. Read packet from TUN.
2. Send packet to proxy/server over TCP (length-prefixed binary protocol).
3. Receive response packet from proxy/server.
4. Write response packet back to TUN.

The terminal output includes live timestamped logs for each step so you can see what is happening in real time.

## Critical routing note (Linux)

Even if forwarding works, Linux will not send traffic to your TUN interface until routing rules are configured.

Example split-default routing commands:

```bash
sudo ip route add 0.0.0.0/1 dev tun0
sudo ip route add 128.0.0.0/1 dev tun0
```

Adjust `tun0` to match your interface name.
