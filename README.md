# PortBridge

PortBridge is a small C++20 learning project for building a self-hosted tunneling
tool, roughly in the shape of a local-first `ngrok` or `cloudflared`.

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Current CLI Shape

```sh
portbridge --help
portbridge --version
portbridge server --listen 443
portbridge expose --local 3000 --remote myapp.example.com
```

At this stage, `server` and `expose` only parse and report their configuration.
They do not open sockets yet.

## Repository Layout

```text
include/portbridge/  Public headers for reusable project code.
src/                 Implementation files and executable entry point.
tests/               CTest-based unit tests.
CMakeLists.txt       Build definition.
```

The split between `portbridge_core` and the `portbridge` executable is deliberate:
most logic should live in the library so tests can exercise it without shelling
out to a process.

## Protocol Layer

The first internal protocol primitive is a tunnel frame:

```text
type       1 byte
stream_id  4 bytes, unsigned, big-endian
length     4 bytes, unsigned, big-endian
payload    N bytes
```

TCP gives us a byte stream, not message boundaries. Because of that,
`decode_frame` can return `need_more` when only part of a frame has arrived.
Callers keep the unread bytes, append more socket data later, and retry.
