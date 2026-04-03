# Docker - Matt_daemon

## Construir imagen
```bash
docker build -t matt_daemon .
```

## Arrancar contenedor (modo interactivo)
```bash
docker run -it --rm --name matt_test -p 4242:4242 matt_daemon
```

Estarás dentro del contenedor con una shell.

---

## Dentro del contenedor

### Ejecutar daemon
```bash
./Matt_daemon
```

### Ver logs
```bash
tail -f /var/log/matt_daemon/matt_daemon.log
```

### Verificar proceso
```bash
ps -o pid,ppid,pgid,sid,tty,stat,cmd -p $(pgrep Matt_daemon)
```

### Ver file descriptors
```bash
ls -l /proc/$(pgrep Matt_daemon)/fd/
```

### Salir del contenedor
```bash
exit
```

---

## Desde el HOST (otra terminal)

### Conectar con nc
```bash
nc localhost 4242
```

### Conectar con Ben_AFK
```bash
./Ben_AFK
```

---

## Gestión del contenedor

### Abrir otra shell en el contenedor que ya está corriendo
```bash
docker exec -it matt_test bash
```

### Parar contenedor
```bash
docker stop matt_test
```

### Parar contenedor inmediatamente (forzar)
```bash
docker kill matt_test
```

### Eliminar contenedor parado
```bash
docker rm matt_test
```

### Parar y eliminar en un comando
```bash
docker stop matt_test && docker rm matt_test
```

---

## Reiniciar

### Reiniciar contenedor que ya existe
```bash
docker restart matt_test
```

### Parar, eliminar y arrancar de nuevo
```bash
docker stop matt_test
docker rm matt_test
docker run -it --rm --name matt_test -p 4242:4242 matt_daemon
```

---

## Limpieza del sistema

### Eliminar imagen
```bash
docker rmi matt_daemon
```

### Eliminar todos los contenedores parados
```bash
docker container prune
```

### Eliminar todas las imágenes sin usar
```bash
docker image prune -a
```

### Limpiar todo (contenedores, imágenes, redes, caché)
```bash
docker system prune -a
```

### Limpiar TODO incluyendo volúmenes
```bash
docker system prune -a --volumes
```

### Ver espacio usado por Docker
```bash
docker system df
```

---

## Listar y verificar

### Ver contenedores corriendo
```bash
docker ps
```

### Ver todos los contenedores (incluidos parados)
```bash
docker ps -a
```

### Ver imágenes
```bash
docker images
```

### Ver logs del contenedor
```bash
docker logs matt_test
```

### Ver logs en tiempo real
```bash
docker logs -f matt_test
```

### Ver procesos dentro del contenedor
```bash
docker top matt_test
```

### Inspeccionar contenedor
```bash
docker inspect matt_test
```

---

## Testing y Validación

### Test 0:
``` bash
# Ocupar el puerto
nc -l 4242

# En otra terminal - Intentar arrancar daemon
./Matt_daemon
# Esperado: Error en bind()
# "bind() failed: Address already in use" deberia guardarse en log
```
### Test 1: Verificar que solo corre una instancia (flock)
```bash
# Dentro del contenedor
./Matt_daemon

# Abrir otra shell en el mismo contenedor
docker exec -it matt_test bash
./Matt_daemon
# Esperado: Error "Another instance is already running"
# Esperado: "Can't open :/var/lock/matt_daemon.lock"

# Verificar logs
tail /var/log/matt_daemon/matt_daemon.log
# Esperado: [ ERROR ] - Error file locked.
```

### Test 2: Límite de 3 clientes
```bash
# Dentro del contenedor - Arrancar daemon
./Matt_daemon

# Desde el HOST - Abrir 4 terminales y conectar
# Terminal 1: nc localhost 4242  # Cliente 1 ✓
# Terminal 2: nc localhost 4242  # Cliente 2 ✓
# Terminal 3: nc localhost 4242  # Cliente 3 ✓
# Terminal 4: nc localhost 4242  # Cliente 4 ✗ (rechazado)

# Verificar logs (dentro del contenedor)
tail /var/log/matt_daemon/matt_daemon.log
# Esperado: [ WARNING ] - Maximum clients reached, rejecting connection
```

### Test 3: Señales capturables (shutdown limpio)
```bash
# SIGTERM (15) - Terminación normal
pkill -SIGTERM Matt_daemon
# o: pkill -15 Matt_daemon
# o: pkill Matt_daemon  (SIGTERM es default)

# SIGINT (2) - Interrupt (Ctrl+C)
pkill -SIGINT Matt_daemon
# o: pkill -2 Matt_daemon

# SIGQUIT (3) - Quit (Ctrl+\)
pkill -SIGQUIT Matt_daemon
# o: pkill -3 Matt_daemon

# SIGHUP (1) - Hangup
pkill -SIGHUP Matt_daemon
# o: pkill -1 Matt_daemon

# Verificar logs después de cada señal
tail -5 /var/log/matt_daemon/matt_daemon.log
# Esperado:
# [ INFO ] - Received signal: SIGxxx
# [ INFO ] - Signal handler.
# [ INFO ] - All clients disconnected
# [ INFO ] - Server stopped
# [ INFO ] - Lock file removed
# [ INFO ] - Quitting.
```

### Test 4: Señales NO capturables
```bash
# SIGKILL (9) - Kill inmediato (NO se captura)
./Matt_daemon
pkill -9 Matt_daemon
# o: pkill -SIGKILL Matt_daemon
# o: pkill -KILL Matt_daemon

# Ver estado del lock file
ls -l /var/lock/matt_daemon.lock
# Esperado: Archivo EXISTE (rw-r--r-- 1 root root 0 ...)

lslocks | grep matt_daemon
# Esperado: VACÍO (lock liberado por el kernel)

# Verificar logs - NO hay shutdown limpio
tail -5 /var/log/matt_daemon/matt_daemon.log
# Esperado: última línea es "Server running", sin "Quitting."

# SIGSTOP (19) - Stop inmediato (pausa el proceso)
./Matt_daemon
pkill -STOP Matt_daemon
# o: pkill -19 Matt_daemon
# o: pkill -SIGSTOP Matt_daemon

# Verificar estado
ps aux | grep Matt_daemon
# Esperado: Estado = T (stopped)

# Continuar el proceso
pkill -CONT Matt_daemon

# Matar para limpiar
pkill -9 Matt_daemon
```

### Test 5: Verificar daemon (proceso correcto)
```bash
# Ver información completa del proceso
ps -o pid,ppid,pgid,sid,tty,stat,cmd -p $(pgrep Matt_daemon)

# Esperado en Docker:
#   PID  PPID  PGID   SID TT   STAT CMD
#     X     1     X     X ?    S    ./Matt_daemon
#           ↑                  ↑
#           init               Sin terminal
```

### Test 6: Verificar directorio de trabajo
```bash
# Verificar que el directorio de trabajo es /
pwdx $(pgrep Matt_daemon)
# Esperado: <PID>: /

# O método alternativo:
readlink /proc/$(pgrep Matt_daemon)/cwd
# Esperado: /
```

### Test 7: Verificar file descriptors
```bash
# Ver FDs del daemon
ls -l /proc/$(pgrep Matt_daemon)/fd/

# Esperado:
# 0 -> /dev/null  (stdin)
# 1 -> /dev/null  (stdout)
# 2 -> /dev/null  (stderr)
# 3 -> /var/log/matt_daemon/matt_daemon.log
# 4 -> socket:[...] (server socket)
# 5 -> anon_inode:[signalfd]
# 6+ -> conexiones de clientes (si hay)
```

### Test 8: Verificar sockets activos
```bash
# Ver sockets TCP del daemon
lsof -p $(pgrep Matt_daemon) | grep TCP

# Esperado (sin clientes):
# Matt_daem <PID> root 4u IPv4 ... TCP *:4242 (LISTEN)

# Esperado (con 3 clientes):
# Matt_daem <PID> root 4u IPv4 ... TCP *:4242 (LISTEN)
# Matt_daem <PID> root 6u IPv4 ... TCP localhost:4242->localhost:XXXXX (ESTABLISHED)
# Matt_daem <PID> root 7u IPv4 ... TCP localhost:4242->localhost:XXXXX (ESTABLISHED)
# Matt_daem <PID> root 8u IPv4 ... TCP localhost:4242->localhost:XXXXX (ESTABLISHED)
```

### Test 9: Log rotation (stress test)
```bash
# Arrancar daemon
./Matt_daemon

# Desde el HOST - Enviar muchos mensajes grandes
while true; do echo "Message XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX $(date +%s%N)"; sleep 0.01; done | nc localhost 4242

# Dentro del contenedor - Verificar rotación
ls -lh /var/log/matt_daemon/
# Esperado: archivos rotados aparecen (matt_daemon.log.YYYYMMDD_HHMMSS)

# Parar el test (Ctrl+C en la terminal del while)

# Limpiar daemon
pkill Matt_daemon
```

### Test 10: Comando quit
```bash
# Arrancar daemon
./Matt_daemon

# Desde el HOST
nc localhost 4242
Hello
World
quit

# Verificar que el daemon se cerró
ps aux | grep Matt_daemon
# Esperado: NO aparece (solo el grep)

# Verificar logs
tail -10 /var/log/matt_daemon/matt_daemon.log
# Esperado:
# [ LOG ] - User input: Hello
# [ LOG ] - User input: World
# [ INFO ] - Request quit.
# [ INFO ] - All clients disconnected
# [ INFO ] - Server stopped
# [ INFO ] - Lock file removed
# [ INFO ] - Quitting.
```

### Test 11: Mensajes que contienen "quit" pero no son exactamente "quit"
```bash
# Arrancar daemon
./Matt_daemon

# Desde el HOST
nc localhost 4242
hola quit
quit hola
quitar
QUIT

# Verificar que el daemon SIGUE corriendo
docker exec -it matt_test ps aux | grep Matt_daemon
# Esperado: Proceso existe

# Verificar logs - Ninguno debe cerrar el daemon
docker exec -it matt_test tail /var/log/matt_daemon/matt_daemon.log
# Esperado:
# [ LOG ] - User input: hola quit
# [ LOG ] - User input: quit hola
# [ LOG ] - User input: quitar
# [ LOG ] - User input: QUIT

# Cerrar correctamente
echo "quit" | nc localhost 4242
```

---

## Ejemplo de uso completo

```bash
# Terminal 1 - Arrancar
docker run -it --rm --name matt_test -p 4242:4242 matt_daemon
./Matt_daemon
tail -f /var/log/matt_daemon/matt_daemon.log

# Terminal 2 - Conectar con nc
nc localhost 4242
Hello
quit

# Terminal 3 - Conectar con cliente
./Ben_AFK
Test
quit
```

---

## Troubleshooting

### "Port 4242 already in use"
```bash
# Ver qué proceso usa el puerto
sudo lsof -i :4242

# Matar el proceso
sudo kill -9 <PID>

# O parar contenedores que usen ese puerto
docker ps | grep matt | awk '{print $1}' | xargs docker stop
```

### "Container name already in use"
```bash
# Eliminar contenedor existente
docker rm -f matt_test

# O usar otro nombre
docker run -it --rm --name matt_test2 -p 4242:4242 matt_daemon
```

### Contenedor se cierra solo
```bash
# Verificar que usas -it (modo interactivo)
docker run -it --rm --name matt_test -p 4242:4242 matt_daemon

# Ver logs de errores
docker logs matt_test
```

### No puedo conectar desde el host
```bash
# Verificar que el puerto está mapeado
docker ps
# Buscar: 0.0.0.0:4242->4242/tcp

# Verificar que el daemon está corriendo dentro
docker exec -it matt_test ps aux | grep Matt
```

---

## Comandos rápidos de limpieza

### Reset completo (empezar de cero)
```bash
# Parar todo
docker stop $(docker ps -aq)

# Eliminar todo
docker rm $(docker ps -aq)

# Limpiar sistema
docker system prune -a

# Reconstruir
docker build -t matt_daemon .
```

### Limpiar solo contenedores de matt_daemon
```bash
docker ps -a | grep matt | awk '{print $1}' | xargs docker rm -f
```

### Ver cuánto espacio ocupa Docker
```bash
docker system df
```