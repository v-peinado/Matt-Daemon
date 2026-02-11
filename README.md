# Matt_daemon

Unix daemon that listens for TCP connections and logs messages to a file.

## Description

Matt_daemon is a background service that accepts up to 3 simultaneous connections on port 4242 and stores all received messages with timestamps. It implements the classic Unix daemonization pattern with double fork, signal handling via signalfd, and guarantees single-instance through lock file.

## Features

- Background execution (real daemon, not a process with `&`)
- Single-instance via lock file with `flock()`
- Up to 3 simultaneous clients
- Timestamped logging to `/var/log/matt_daemon/matt_daemon.log`
- Clean shutdown on signals (SIGTERM, SIGINT, SIGQUIT, SIGHUP)
- Remote shutdown via `quit` command
- Need Root privileges

## Installation

```bash
git clone <repository>
cd matt_daemon
make
```

## Usage (Native Root)

Use this method if you have root access on your machine.

### Start the daemon

```bash
sudo ./Matt_daemon
```

### Connect and send messages

```bash
nc localhost 4242
Hello world
This message gets logged
quit
```

### Check status

```bash
# Check if running
ps aux | grep Matt_daemon

# Check lock file
ls -l /var/lock/matt_daemon.lock

# View logs
tail -f /var/log/matt_daemon/matt_daemon.log
```

### Stop the daemon

```bash
# Option 1: Send quit
echo "quit" | nc localhost 4242

# Option 2: Signal
sudo kill -SIGTERM $(pgrep Matt_daemon)
```

## Usage (Docker)

Use this method if you don't have root access or prefer containerized execution.

### Build image

```bash
docker build -t matt_daemon .
```

### Start container

```bash
sudo docker run -it --name matt_daemon -p 4242:4242 matt_daemon bash
```

### Inside the container

```bash
# Start daemon
./Matt_daemon

# Verify it's running
ps aux | grep Matt

# View logs
cat /var/log/matt_daemon/matt_daemon.log

# Follow logs in real time
tail -f /var/log/matt_daemon/matt_daemon.log
```

### From host (another terminal)

```bash
# Connect to daemon
nc localhost 4242

# Send messages
Hello world
quit
```

### Container management

```bash
# Exit container (daemon keeps running inside)
# Press Ctrl+P, Ctrl+Q

# Re-enter container
sudo docker exec -it matt_daemon bash

# Stop and remove container
sudo docker stop matt_daemon
sudo docker rm matt_daemon
```

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                        main()                           │
│                          │                              │
│    ┌─────────────────────┼─────────────────────┐        │
│    ▼                     ▼                     ▼        │
│ TintinReporter      MattDaemon              Config      │
│ (logging)           (orchestrator)        (constants)   │
│                          │                              │
│            ┌─────────────┼─────────────────────┐        │
│            ▼                           ▼                │
│        Daemonize                    Server              │
│     (fork, setsid)            (TCP, select, signals)    │
└─────────────────────────────────────────────────────────┘
```

| Component | Responsibility |
|-----------|----------------|
| `TintinReporter` | Timestamped log writing |
| `MattDaemon` | Root verification, lock file, lifecycle |
| `Server` | TCP socket, I/O multiplexing with select(), signals |
| `Daemonize` | Double fork, setsid, FD redirection |
| `Config` | Constants: port, paths, limits |

## Execution Flow

```
main()
  │
  ├─► TintinReporter()           # Create logger
  │     └─► mkdir(), open()
  │
  ├─► MattDaemon::init()
  │     ├─► getuid()             # Verify root
  │     ├─► open(), flock()      # Lock file
  │     └─► Server::init()
  │           ├─► socket()
  │           ├─► bind()
  │           ├─► listen()
  │           └─► signalfd()
  │
  └─► MattDaemon::run()
        ├─► Daemonize::daemonize()
        │     ├─► fork() + fork()
        │     ├─► setsid()
        │     ├─► chdir("/")
        │     └─► dup2() → /dev/null
        │
        └─► Server::run()        # Main loop
              └─► select()
                    ├─► signal  → shutdown
                    ├─► new client → accept()
                    └─► data → recv() → log
```

## Signals

| Signal | Behavior |
|--------|----------|
| SIGTERM | Log + clean shutdown |
| SIGINT | Log + clean shutdown |
| SIGQUIT | Log + clean shutdown |
| SIGHUP | Log + clean shutdown |
| SIGKILL | Immediate termination (not catchable) |
| SIGSTOP | Immediate pause (not catchable) |

## Logs

**Location:** `/var/log/matt_daemon/matt_daemon.log`

**Format:**
```
[DD/MM/YYYY-HH:MM:SS] [ LEVEL ] - Matt_daemon: message
```

**Levels:**

| Level | Usage |
|-------|-------|
| INFO | System events (startup, shutdown, connections) |
| LOG | User messages |
| WARNING | Non-critical issues |
| ERROR | Critical errors |

**Log example:**
```
[11/02/2026-11:20:04] [ INFO ] - Matt_daemon: Server object created
[11/02/2026-11:20:04] [ INFO ] - Matt_daemon: Started.
[11/02/2026-11:20:04] [ INFO ] - Matt_daemon: Root privileges confirmed
[11/02/2026-11:20:04] [ INFO ] - Matt_daemon: Lock file created: /var/lock/matt_daemon.lock
[11/02/2026-11:20:04] [ INFO ] - Matt_daemon: Creating server...
[11/02/2026-11:20:04] [ INFO ] - Matt_daemon: Socket created
[11/02/2026-11:20:04] [ INFO ] - Matt_daemon: Socket bound to port 4242
[11/02/2026-11:20:04] [ INFO ] - Matt_daemon: Server listening on port 4242
[11/02/2026-11:20:04] [ INFO ] - Matt_daemon: Signal handling configured (signalfd)
[11/02/2026-11:20:04] [ INFO ] - Matt_daemon: Server created.
[11/02/2026-11:20:04] [ INFO ] - Matt_daemon: Standard FDs redirected to /dev/null
[11/02/2026-11:20:04] [ INFO ] - Matt_daemon: Entering Daemon mode.
[11/02/2026-11:20:04] [ INFO ] - Matt_daemon: started. PID: 12.
[11/02/2026-11:20:04] [ INFO ] - Matt_daemon: Server running
[11/02/2026-11:20:43] [ INFO ] - Matt_daemon: Client connected from 172.17.0.1:47874
[11/02/2026-11:20:47] [ LOG ] - Matt_daemon: User input: Hello world
[11/02/2026-11:21:13] [ INFO ] - Matt_daemon: Request quit.
[11/02/2026-11:21:13] [ INFO ] - Matt_daemon: All clients disconnected
[11/02/2026-11:21:13] [ INFO ] - Matt_daemon: Server stopped
[11/02/2026-11:21:13] [ INFO ] - Matt_daemon: Lock file removed
[11/02/2026-11:21:13] [ INFO ] - Matt_daemon: Quitting.
```

## System Files

| Path | Purpose |
|------|---------|
| `/var/lock/matt_daemon.lock` | Lock file (single instance) |
| `/var/log/matt_daemon/matt_daemon.log` | Log file |

## Limitations

- IPv4 only
- No encryption (plain text)
- No authentication
- Maximum 3 simultaneous clients
- Requires root

## Project Structure

```
matt_daemon/
├── Dockerfile
├── Makefile
├── README.md
├── includes/
│   ├── Config.hpp
│   ├── Daemonize.hpp
│   ├── MattDaemon.hpp
│   ├── Server.hpp
│   └── TintinReporter.hpp
└── src/
    ├── Daemonize.cpp
    ├── MattDaemon.cpp
    ├── Server.cpp
    ├── TintinReporter.cpp
    └── main.cpp
```