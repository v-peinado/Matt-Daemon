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
