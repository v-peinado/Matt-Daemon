## Cheat Sheet Docker

### Construir imagen
```bash
docker build -t matt_daemon .
```
### Arrancar contenedor en modo interactivo
```bash
docker run -it --rm --name matt_test -p 4242:4242 matt_daemon
```
### Abrir otra shell del contenedor, para conectar
```bash
docker exec -it matt_test bash
```
### Comandos basicos Docker
```bash
docker ps                   # Ver contenedores corriendo, anadir [-a] para incluir los parados
docker stop matt_test       # Parar
docker kill matt_test       # Forzar
docker rm matt_test         # Eliminar contenedor parado  
docker restart matt_test    # Reiniciar
```
### Limpieza de sistema
```bash
docker rmi matt_daemon  # Eliminar imagen
docker container prune  # Eliminar todos los contenedores parados
docker image prune -a   # Eliminar todas las imágenes sin usar
docker system prune -a # Limpiar todo (contenedores, imágenes, redes, caché)
docker system prune -a --volumes # Limpiar TODO incluyendo volúmenes
```
## Pruebas rapidas del proyecto

### Verificarificaciones del proceso
```bash
# Ver información completa del proceso, ver su proceso padre(1) y si esta asociado a una tty(?), stat(s, sin terminal)
ps -o pid,ppid,tty,stat,cmd -p $(pgrep Matt_daemon)

# Verificar que el directorio de trabajo es /
pwdx $(pgrep Matt_daemon)

# Ver FDs del daemon
ls -l /proc/$(pgrep Matt_daemon)/fd/

# Verificar sockets TCP activos, es posible que si no tienes lsof instalado no funcione
lsof -p $(pgrep Matt_daemon) | grep TCP

# Verificar que el proceso se acaba cuando usas signals o quit
ps aux | grep Matt_daemon     # El propio comando grep aparecera, es normal

```
### Generar un warning al intentar bind de un puerto ya usado (antes de ejecutar ./Matt_daemon)
```bash
nc -l 4242 # Ocupar el puerto
./Matt_daemon
# "bind() failed: Address already in use" deberia guardarse en log
```
### Abrir consola de logs en bucle
```bash
tail -f /var/log/matt_daemon/matt_daemon.log
```
### Signals
```bash
pkill -SIGTERM Matt_daemon      # SIGTERM (15) - Terminación normal
# o: pkill -15 Matt_daemon
# o: pkill Matt_daemon  (SIGTERM es default)

pkill -SIGINT Matt_daemon       # SIGINT (2) - Interrupt (Ctrl+C)
# o: pkill -2 Matt_daemon

pkill -SIGQUIT Matt_daemon      # SIGQUIT (3) - Quit (Ctrl+\)
# o: pkill -3 Matt_daemon

pkill -SIGHUP Matt_daemon       # SIGHUP (1) - Hangup
# o: pkill -1 Matt_daemon

pkill -9 Matt_daemon            # Signal no capturable
# o: pkill -SIGKILL Matt_daemon
# o: pkill -KILL Matt_daemon

ps aux | grep Matt_daemon # Verificar estado
```

### Logs rotate
```bash
# Desde el HOST - Enviar muchos mensajes grandes
while true; do echo "Message XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX $(date +%s%N)"; sleep 0.01; done | nc localhost 4242

# Dentro del contenedor - Verificar rotación
ls -lh /var/log/matt_daemon/
# Esperado: archivos rotados aparecen (matt_daemon.log.YYYYMMDD_HHMMSS)

# Parar el test (Ctrl+C en la terminal del while)
```
