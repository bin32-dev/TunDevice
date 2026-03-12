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

Non-interactive CLI usage (no prompts):

```bash
sudo ./build/bin/tun_proxy_ui \
  --tun tun0 \
  --cidr 10.20.0.1/24 \
  --proxy 192.168.65.183 \
  --port 8000 \
  --protocol http \
  --http-path /
```

- `--protocol http` is now the default (good for HTTP-based packet gateway endpoints).
- Use `--protocol binary` if your server uses the older length-prefixed raw TCP protocol.

> Creating/configuring TUN devices usually requires root privileges.

## Forwarding flow (implemented)

The app now runs the real forwarding loop:

1. Read packet from TUN.
2. Send packet to proxy/server (HTTP POST by default, or legacy length-prefixed binary when `--protocol binary` is used).
3. Receive response packet/body from proxy/server.
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
