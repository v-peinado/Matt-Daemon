# Matt_daemon

[![Language](https://img.shields.io/badge/language-C%2B%2B20-blue.svg)](https://isocpp.org/)

A production-grade UNIX daemon that listens for TCP connections on port 4242 and logs messages with timestamps. Implements proper daemonization with double fork, signal handling via `signalfd`, single-instance enforcement with `flock`, and I/O multiplexing with `select()`.

## Quick Start

### Build
```bash
make              # Build daemon only
make bonus        # Build daemon + client
```

### Run (requires root)
```bash
sudo ./Matt_daemon
```

### Connect
```bash
# Using netcat
nc localhost 4242

# Or using the included client
./Ben_AFK
```

### Stop
```bash
echo "quit" | nc localhost 4242
# or
sudo pkill -SIGTERM Matt_daemon
```

## Features

### Server (Matt_daemon)
- ✅ True daemonization (double fork, setsid, FD redirection)
- ✅ Single instance via `flock()` on `/var/lock/matt_daemon.lock`
- ✅ Handles up to 3 simultaneous TCP connections
- ✅ Advanced logging with automatic rotation and cleanup
- ✅ Signal handling (SIGTERM, SIGINT, SIGQUIT, SIGHUP)
- ✅ I/O multiplexing with `select()`

### Client (Ben_AFK)
- ✅ Interactive TCP client with signal handling
- ✅ Connection monitoring and auto-detection of server shutdown
- ✅ I/O multiplexing for stdin and socket
- ✅ Clean graceful exit

### Logger (TintinReporter)
- ✅ Timestamped entries: `[DD/MM/YYYY-HH:MM:SS]`
- ✅ Automatic rotation at 10 MB
- ✅ Auto-cleanup of logs older than 30 days
- ✅ Four severity levels (INFO, LOG, WARNING, ERROR)

## Architecture

### Server
```
main() → TintinReporter (logging + rotation)
      → MattDaemon (lock file + orchestration)
      → Server (TCP + select + signalfd)
      → Daemonize (fork + setsid + FD redirect)
```

### Client
```
main() → BenAfk (orchestrator + select)
      → Connection (TCP socket)
      → signalfd (SIGINT/SIGTERM)
```

## Components

| Component | Purpose |
|-----------|---------|
| **TintinReporter** | Advanced logger with rotation, cleanup, 4 severity levels |
| **MattDaemon** | Root verification, lock file (`flock`), lifecycle management |
| **Server** | TCP socket (port 4242), `select()` I/O mux, `signalfd` signals |
| **Daemonize** | Double fork, `setsid()`, working dir to `/`, FD → `/dev/null` |
| **BenAfk** | Client orchestrator with signal handling and I/O multiplexing |
| **Connection** | TCP connection wrapper (connect, send, disconnect) |

## System Files

| Path | Purpose |
|------|---------|
| `/var/lock/matt_daemon.lock` | Single-instance lock file |
| `/var/log/matt_daemon/matt_daemon.log` | Current log |
| `/var/log/matt_daemon/matt_daemon.log.*` | Rotated logs |

## Logging

**Format:** `[DD/MM/YYYY-HH:MM:SS] [ LEVEL ] - Matt_daemon: message`

**Levels:** INFO (system events) | LOG (user input) | WARNING (non-critical) | ERROR (critical)

**Sample:**
```
[11/02/2026-11:20:04] [ INFO ] - Matt_daemon: Started.
[11/02/2026-11:20:04] [ INFO ] - Matt_daemon: started. PID: 12.
[11/02/2026-11:20:43] [ INFO ] - Matt_daemon: Client connected from 172.17.0.1:47874
[11/02/2026-11:20:47] [ LOG ] - Matt_daemon: User input: Hello world
[11/02/2026-11:21:13] [ INFO ] - Matt_daemon: Request quit.
```

## Docker Usage

```bash
# Build
docker build -t matt_daemon .

# Run
docker run -it --rm --name matt_test -p 4242:4242 matt_daemon bash
./Matt_daemon

# Connect from host
nc localhost 4242
```

## Signals

| Signal | Action | Catchable |
|--------|--------|-----------|
| SIGTERM (15) | Graceful shutdown | ✅ |
| SIGINT (2) | Graceful shutdown | ✅ |
| SIGQUIT (3) | Graceful shutdown | ✅ |
| SIGHUP (1) | Graceful shutdown | ✅ |
| SIGKILL (9) | Immediate kill | ❌ |
| SIGSTOP (19) | Immediate pause | ❌ |

## Project Structure

```
matt_daemon/
├── Makefile
├── README.md
├── Dockerfile
├── docs/
│   ├── Extended-Commands-Reference.md    # Comprehensive testing guide
│   └── Essential-Commands-Cheat-Sheet.md # Quick command reference
├── server/
│   ├── includes/
│   │   ├── Daemonize.hpp
│   │   ├── MattDaemon.hpp
│   │   ├── Server.hpp
│   │   └── TintinReporter.hpp
│   └── src/
│       ├── Daemonize.cpp
│       ├── MattDaemon.cpp
│       ├── Server.cpp
│       ├── TintinReporter.cpp
│       └── main.cpp
└── client/
    ├── includes/
    │   ├── BenAfk.hpp
    │   └── Connection.hpp
    └── src/
        ├── BenAfk.cpp
        ├── Connection.cpp
        └── main.cpp
```

## Documentation

- **[Extended Commands Reference](docs/Extended-Commands-Reference.md)** - Complete Docker workflow, testing procedures, troubleshooting
- **[Essential Commands Cheat Sheet](docs/Essential-Commands-Cheat-Sheet.md)** - Quick reference for common operations

## Requirements

- Linux kernel ≥ 3.14
- C++20 compiler (g++, clang++)
- Root privileges (UID 0)
- POSIX system calls: `fork`, `setsid`, `flock`, `signalfd`, `select`

## Technical Highlights

- **Double Fork Pattern** - Ensures complete detachment from controlling terminal
- **File Locking** - `flock(LOCK_EX | LOCK_NB)` for single-instance enforcement
- **Signal FD** - Modern signal handling via `signalfd()` instead of traditional signal handlers
- **I/O Multiplexing** - `select()` for concurrent client and signal monitoring
- **Log Rotation** - Automatic size-based rotation with timestamp-based naming
- **Clean Shutdown** - Proper resource cleanup, lock file removal, client disconnection

## Limitations

- IPv4 only
- No encryption (plain text)
- No authentication
- Max 3 concurrent clients
- Single-threaded (uses `select()` for concurrency)
