# Docker - Matt_daemon

## Build Image
```bash
docker build -t matt_daemon .
```

## Start Container (Interactive Mode)
```bash
docker run -it --rm --name matt_test -p 4242:4242 matt_daemon
```

You'll be inside the container with a shell.

---

## Inside the Container

### Run Daemon
```bash
./Matt_daemon
```

### View Logs
```bash
tail -f /var/log/matt_daemon/matt_daemon.log
```

### Verify Process
```bash
ps -o pid,ppid,pgid,sid,tty,stat,cmd -p $(pgrep Matt_daemon)
```

### View File Descriptors
```bash
ls -l /proc/$(pgrep Matt_daemon)/fd/
```

### Exit Container
```bash
exit
```

---

## From HOST (Another Terminal)

### Connect with nc
```bash
nc localhost 4242
```

### Connect with Ben_AFK
```bash
./Ben_AFK
```

---

## Container Management

### Open Another Shell in Running Container
```bash
docker exec -it matt_test bash
```

### Stop Container
```bash
docker stop matt_test
```

### Force Stop Container
```bash
docker kill matt_test
```

### Remove Stopped Container
```bash
docker rm matt_test
```

### Stop and Remove in One Command
```bash
docker stop matt_test && docker rm matt_test
```

---

## Restart

### Restart Existing Container
```bash
docker restart matt_test
```

### Stop, Remove and Start Again
```bash
docker stop matt_test
docker rm matt_test
docker run -it --rm --name matt_test -p 4242:4242 matt_daemon
```

---

## System Cleanup

### Remove Image
```bash
docker rmi matt_daemon
```

### Remove All Stopped Containers
```bash
docker container prune
```

### Remove All Unused Images
```bash
docker image prune -a
```

### Clean Everything (containers, images, networks, cache)
```bash
docker system prune -a
```

### Clean EVERYTHING Including Volumes
```bash
docker system prune -a --volumes
```

### View Docker Disk Usage
```bash
docker system df
```

---

## List and Verify

### View Running Containers
```bash
docker ps
```

### View All Containers (Including Stopped)
```bash
docker ps -a
```

### View Images
```bash
docker images
```

### View Container Logs
```bash
docker logs matt_test
```

### View Logs in Real Time
```bash
docker logs -f matt_test
```

### View Processes Inside Container
```bash
docker top matt_test
```

### Inspect Container
```bash
docker inspect matt_test
```

---

## Testing and Validation

### Test 0:
``` bash
# Occupy the port
nc -l 4242

# In another terminal - Try to start daemon
./Matt_daemon
# Expected: Error in bind()
# "bind() failed: Address already in use" should be saved in log
```

### Test 1: Verify Only One Instance Runs (flock)
```bash
# Inside the container
./Matt_daemon

# Open another shell in the same container
docker exec -it matt_test bash
./Matt_daemon
# Expected: Error "Another instance is already running"
# Expected: "Can't open :/var/lock/matt_daemon.lock"

# Verify logs
tail /var/log/matt_daemon/matt_daemon.log
# Expected: [ ERROR ] - Error file locked.
```

### Test 2: 3 Client Limit
```bash
# Inside the container - Start daemon
./Matt_daemon

# From the HOST - Open 4 terminals and connect
# Terminal 1: nc localhost 4242  # Client 1 ✓
# Terminal 2: nc localhost 4242  # Client 2 ✓
# Terminal 3: nc localhost 4242  # Client 3 ✓
# Terminal 4: nc localhost 4242  # Client 4 ✗ (rejected)

# Verify logs (inside the container)
tail /var/log/matt_daemon/matt_daemon.log
# Expected: [ WARNING ] - Maximum clients reached, rejecting connection
```

### Test 3: Catchable Signals (Clean Shutdown)
```bash
# SIGTERM (15) - Normal termination
pkill -SIGTERM Matt_daemon
# or: pkill -15 Matt_daemon
# or: pkill Matt_daemon  (SIGTERM is default)

# SIGINT (2) - Interrupt (Ctrl+C)
pkill -SIGINT Matt_daemon
# or: pkill -2 Matt_daemon

# SIGQUIT (3) - Quit (Ctrl+\)
pkill -SIGQUIT Matt_daemon
# or: pkill -3 Matt_daemon

# SIGHUP (1) - Hangup
pkill -SIGHUP Matt_daemon
# or: pkill -1 Matt_daemon

# Verify logs after each signal
tail -5 /var/log/matt_daemon/matt_daemon.log
# Expected:
# [ INFO ] - Received signal: SIGxxx
# [ INFO ] - Signal handler.
# [ INFO ] - All clients disconnected
# [ INFO ] - Server stopped
# [ INFO ] - Lock file removed
# [ INFO ] - Quitting.
```

### Test 4: Non-Catchable Signals
```bash
# SIGKILL (9) - Immediate kill (NOT caught)
./Matt_daemon
pkill -9 Matt_daemon
# or: pkill -SIGKILL Matt_daemon
# or: pkill -KILL Matt_daemon

# Check lock file status
ls -l /var/lock/matt_daemon.lock
# Expected: File EXISTS (rw-r--r-- 1 root root 0 ...)

lslocks | grep matt_daemon
# Expected: EMPTY (lock released by kernel)

# Verify logs - NO clean shutdown
tail -5 /var/log/matt_daemon/matt_daemon.log
# Expected: last line is "Server running", no "Quitting."

# SIGSTOP (19) - Immediate stop (pauses the process)
./Matt_daemon
pkill -STOP Matt_daemon
# or: pkill -19 Matt_daemon
# or: pkill -SIGSTOP Matt_daemon

# Verify state
ps aux | grep Matt_daemon
# Expected: State = T (stopped)

# Continue the process
pkill -CONT Matt_daemon

# Kill to clean up
pkill -9 Matt_daemon
```

### Test 5: Verify Daemon (Correct Process)
```bash
# View complete process information
ps -o pid,ppid,pgid,sid,tty,stat,cmd -p $(pgrep Matt_daemon)

# Expected in Docker:
#   PID  PPID  PGID   SID TT   STAT CMD
#     X     1     X     X ?    S    ./Matt_daemon
#           ↑                  ↑
#           init               No terminal
```

### Test 6: Verify Working Directory
```bash
# Verify working directory is /
pwdx $(pgrep Matt_daemon)
# Expected: <PID>: /

# Or alternative method:
readlink /proc/$(pgrep Matt_daemon)/cwd
# Expected: /
```

### Test 7: Verify File Descriptors
```bash
# View daemon's FDs
ls -l /proc/$(pgrep Matt_daemon)/fd/

# Expected:
# 0 -> /dev/null  (stdin)
# 1 -> /dev/null  (stdout)
# 2 -> /dev/null  (stderr)
# 3 -> /var/log/matt_daemon/matt_daemon.log
# 4 -> socket:[...] (server socket)
# 5 -> anon_inode:[signalfd]
# 6+ -> client connections (if any)
```

### Test 8: Verify Active Sockets
```bash
# View daemon's TCP sockets
lsof -p $(pgrep Matt_daemon) | grep TCP

# Expected (without clients):
# Matt_daem <PID> root 4u IPv4 ... TCP *:4242 (LISTEN)

# Expected (with 3 clients):
# Matt_daem <PID> root 4u IPv4 ... TCP *:4242 (LISTEN)
# Matt_daem <PID> root 6u IPv4 ... TCP localhost:4242->localhost:XXXXX (ESTABLISHED)
# Matt_daem <PID> root 7u IPv4 ... TCP localhost:4242->localhost:XXXXX (ESTABLISHED)
# Matt_daem <PID> root 8u IPv4 ... TCP localhost:4242->localhost:XXXXX (ESTABLISHED)
```

### Test 9: Log Rotation (Stress Test)
```bash
# Start daemon
./Matt_daemon

# From the HOST - Send many large messages
while true; do echo "Message XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX $(date +%s%N)"; sleep 0.01; done | nc localhost 4242

# Inside the container - Verify rotation
ls -lh /var/log/matt_daemon/
# Expected: rotated files appear (matt_daemon.log.YYYYMMDD_HHMMSS)

# Stop the test (Ctrl+C in the while terminal)

# Clean up daemon
pkill Matt_daemon
```

### Test 10: Quit Command
```bash
# Start daemon
./Matt_daemon

# From the HOST
nc localhost 4242
Hello
World
quit

# Verify daemon closed
ps aux | grep Matt_daemon
# Expected: Does NOT appear (only grep)

# Verify logs
tail -10 /var/log/matt_daemon/matt_daemon.log
# Expected:
# [ LOG ] - User input: Hello
# [ LOG ] - User input: World
# [ INFO ] - Request quit.
# [ INFO ] - All clients disconnected
# [ INFO ] - Server stopped
# [ INFO ] - Lock file removed
# [ INFO ] - Quitting.
```

### Test 11: Messages Containing "quit" But Not Exactly "quit"
```bash
# Start daemon
./Matt_daemon

# From the HOST
nc localhost 4242
hello quit
quit hello
quitar
QUIT

# Verify daemon STILL running
docker exec -it matt_test ps aux | grep Matt_daemon
# Expected: Process exists

# Verify logs - None should close the daemon
docker exec -it matt_test tail /var/log/matt_daemon/matt_daemon.log
# Expected:
# [ LOG ] - User input: hello quit
# [ LOG ] - User input: quit hello
# [ LOG ] - User input: quitar
# [ LOG ] - User input: QUIT

# Close correctly
echo "quit" | nc localhost 4242
```

---

## Complete Usage Example

```bash
# Terminal 1 - Start
docker run -it --rm --name matt_test -p 4242:4242 matt_daemon
./Matt_daemon
tail -f /var/log/matt_daemon/matt_daemon.log

# Terminal 2 - Connect with nc
nc localhost 4242
Hello
quit

# Terminal 3 - Connect with client
./Ben_AFK
Test
quit
```

---

## Troubleshooting

### "Port 4242 already in use"
```bash
# See what process uses the port
sudo lsof -i :4242

# Kill the process
sudo kill -9 <PID>

# Or stop containers using that port
docker ps | grep matt | awk '{print $1}' | xargs docker stop
```

### "Container name already in use"
```bash
# Remove existing container
docker rm -f matt_test

# Or use another name
docker run -it --rm --name matt_test2 -p 4242:4242 matt_daemon
```

### Container Closes by Itself
```bash
# Verify you use -it (interactive mode)
docker run -it --rm --name matt_test -p 4242:4242 matt_daemon

# View error logs
docker logs matt_test
```

### Cannot Connect from Host
```bash
# Verify port is mapped
docker ps
# Look for: 0.0.0.0:4242->4242/tcp

# Verify daemon is running inside
docker exec -it matt_test ps aux | grep Matt
```

---

## Quick Cleanup Commands

### Complete Reset (Start from Scratch)
```bash
# Stop everything
docker stop $(docker ps -aq)

# Remove everything
docker rm $(docker ps -aq)

# Clean system
docker system prune -a

# Rebuild
docker build -t matt_daemon .
```

### Clean Only matt_daemon Containers
```bash
docker ps -a | grep matt | awk '{print $1}' | xargs docker rm -f
```

### View Docker Disk Usage
```bash
docker system df
```
