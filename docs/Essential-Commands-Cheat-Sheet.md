# Docker Commands Cheat Sheet

## Build Image
```bash
docker build -t matt_daemon .
```

## Start Container in Interactive Mode
```bash
docker run -it --rm --name matt_test -p 4242:4242 matt_daemon
```

## Open Another Shell in Running Container
```bash
docker exec -it matt_test bash
```

## Basic Docker Commands
```bash
docker ps                   # View running containers, add [-a] to include stopped ones
docker stop matt_test       # Stop
docker kill matt_test       # Force stop
docker rm matt_test         # Remove stopped container  
docker restart matt_test    # Restart
```

## System Cleanup
```bash
docker rmi matt_daemon                # Remove image
docker container prune                # Remove all stopped containers
docker image prune -a                 # Remove all unused images
docker system prune -a                # Clean everything (containers, images, networks, cache)
docker system prune -a --volumes      # Clean EVERYTHING including volumes
```

---

## Quick Project Tests

### Process Verification
```bash
# View complete process info, check parent process (1) and if associated with tty (?), stat (s, no terminal)
ps -o pid,ppid,tty,stat,cmd -p $(pgrep Matt_daemon)

# Verify working directory is /
pwdx $(pgrep Matt_daemon)

# View daemon's file descriptors
ls -l /proc/$(pgrep Matt_daemon)/fd/

# Verify active TCP sockets, lsof may not work if not installed
lsof -p $(pgrep Matt_daemon) | grep TCP

# Verify process ends when using signals or quit
ps aux | grep Matt_daemon     # The grep command itself will appear, this is normal
```

### Generate Warning When Trying to Bind an Already Used Port (before running ./Matt_daemon)
```bash
nc -l 4242  # Occupy the port
./Matt_daemon
# "bind() failed: Address already in use" should be saved in log
```

### Open Log Console in Loop
```bash
tail -f /var/log/matt_daemon/matt_daemon.log
```

### Signals
```bash
pkill -SIGTERM Matt_daemon      # SIGTERM (15) - Normal termination
# or: pkill -15 Matt_daemon
# or: pkill Matt_daemon  (SIGTERM is default)

pkill -SIGINT Matt_daemon       # SIGINT (2) - Interrupt (Ctrl+C)
# or: pkill -2 Matt_daemon

pkill -SIGQUIT Matt_daemon      # SIGQUIT (3) - Quit (Ctrl+\)
# or: pkill -3 Matt_daemon

pkill -SIGHUP Matt_daemon       # SIGHUP (1) - Hangup
# or: pkill -1 Matt_daemon

pkill -9 Matt_daemon            # Non-catchable signal
# or: pkill -SIGKILL Matt_daemon
# or: pkill -KILL Matt_daemon

ps aux | grep Matt_daemon       # Verify state
```

### Log Rotation
```bash
# From the HOST - Send many large messages
while true; do echo "Message XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX $(date +%s%N)"; sleep 0.01; done | nc localhost 4242

# Inside the container - Verify rotation
ls -lh /var/log/matt_daemon/
# Expected: rotated files appear (matt_daemon.log.YYYYMMDD_HHMMSS)

# Stop the test (Ctrl+C in the while terminal)
```
