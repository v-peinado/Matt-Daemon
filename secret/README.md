# Matt_daemon

Daemon Unix que escucha conexiones TCP y registra mensajes en un archivo de log.

## Descripción

Matt_daemon es un servicio que corre en background, acepta hasta 3 conexiones simultáneas en el puerto 4242, y almacena todos los mensajes recibidos con timestamp. Implementa el patrón clásico de daemonización Unix con doble fork, manejo de señales mediante signalfd, y garantiza single-instance mediante lock file.

## Características

- Ejecución en background (daemon real, no proceso con `&`)
- Single-instance mediante lock file con `flock()`
- Hasta 3 clientes simultáneos
- Logging con timestamp a `/var/log/matt_daemon/matt_daemon.log`
- Shutdown limpio ante señales (SIGTERM, SIGINT, SIGQUIT, SIGHUP)
- Comando `quit` para apagado remoto

## Requisitos

- Linux (kernel > 3.14)
- GCC/Clang con soporte C++17
- Permisos de root

## Instalación

```bash
git clone <repositorio>
cd matt_daemon
make
```

## Uso

### Iniciar el daemon

```bash
sudo ./Matt_daemon
```

### Conectar y enviar mensajes

```bash
nc localhost 4242
Hola mundo
Este mensaje se loguea
quit
```

### Verificar estado

```bash
# Ver si está corriendo
ps aux | grep Matt_daemon

# Ver lock file
ls -l /var/lock/matt_daemon.lock

# Ver logs
tail -f /var/log/matt_daemon/matt_daemon.log
```

### Detener el daemon

```bash
# Opción 1: Enviar quit
echo "quit" | nc localhost 4242

# Opción 2: Señal
sudo kill -SIGTERM $(pgrep Matt_daemon)
```

## Docker

Para evitar ejecutar como root en el host, puedes usar Docker.

### Construcción

```bash
docker build -t matt_daemon .
```

### Ejecución

```bash
# Iniciar contenedor (daemon arranca automáticamente)
docker run -d --name matt_daemon -p 4242:4242 matt_daemon

# Conectar desde el host
nc localhost 4242

# Ver logs desde fuera
docker exec matt_daemon cat /var/log/matt_daemon/matt_daemon.log

# Shell dentro del contenedor
docker exec -it matt_daemon bash

# Detener
docker stop matt_daemon
docker rm matt_daemon
```

### Comandos útiles dentro del contenedor

```bash
# Ver procesos
ps aux | grep Matt

# Ver lock file
ls -la /var/lock/

# Seguir logs en tiempo real
tail -f /var/log/matt_daemon/matt_daemon.log

# Conectar localmente
nc localhost 4242
```

## Arquitectura

```
┌─────────────────────────────────────────────────────────┐
│                        main()                           │
│                          │                              │
│    ┌─────────────────────┼─────────────────────┐        │
│    ▼                     ▼                     ▼        │
│ TintinReporter      MattDaemon              Config      │
│ (logging)      (orquestador, interfaz)    (constantes)  │
│                          │                              │
│            ┌─────────────┼─────────────┐                │
│            ▼                           ▼                │
│        Daemonize                    Server              │
│     (fork, setsid)            (TCP, select, signals)    │
└─────────────────────────────────────────────────────────┘
```

| Componente | Responsabilidad |
|------------|-----------------|
| `TintinReporter` | Escritura de logs con timestamp |
| `MattDaemon` | Verificación root, lock file, ciclo de vida |
| `Server` | Socket TCP, multiplexación con select(), señales |
| `Daemonize` | Doble fork, setsid, redirección de FDs |
| `Config` | Constantes: puerto, rutas, límites |

## Flujo de Ejecución

```
main()
  │
  ├─► TintinReporter()           # Crear logger
  │     └─► mkdir(), open()
  │
  ├─► MattDaemon::init()
  │     ├─► getuid()             # Verificar root
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
        └─► Server::run()        # Loop principal
              └─► select()
                    ├─► señal  → shutdown
                    ├─► nuevo cliente → accept()
                    └─► datos → recv() → log
```

## Señales

| Señal | Comportamiento |
|-------|----------------|
| SIGTERM | Log + shutdown limpio |
| SIGINT | Log + shutdown limpio |
| SIGQUIT | Log + shutdown limpio |
| SIGHUP | Log + shutdown limpio |
| SIGKILL | Terminación inmediata (no manejable) |
| SIGSTOP | Pausa inmediata (no manejable) |

## Logs

**Ubicación:** `/var/log/matt_daemon/matt_daemon.log`

**Formato:**
```
[DD/MM/YYYY-HH:MM:SS] [ LEVEL ] - Matt_daemon: mensaje
```

**Niveles:**

| Nivel | Uso |
|-------|-----|
| INFO | Eventos del sistema (inicio, shutdown, conexiones) |
| LOG | Mensajes de usuarios |
| WARNING | Problemas no críticos |
| ERROR | Errores críticos |

**Ejemplo de log:**
```
[11/01/2026-14:34:58] [ INFO ] - Matt_daemon: Started.
[11/01/2026-14:34:58] [ INFO ] - Matt_daemon: Creating server.
[11/01/2026-14:34:58] [ INFO ] - Matt_daemon: Server created.
[11/01/2026-14:34:58] [ INFO ] - Matt_daemon: Entering Daemon mode.
[11/01/2026-14:34:58] [ INFO ] - Matt_daemon: started. PID: 6498.
[11/01/2026-14:35:10] [ LOG ] - Matt_daemon: User input: Hola mundo
[11/01/2026-14:35:15] [ INFO ] - Matt_daemon: Request quit.
[11/01/2026-14:35:15] [ INFO ] - Matt_daemon: Quitting.
```

## Archivos del Sistema

| Ruta | Propósito |
|------|-----------|
| `/var/lock/matt_daemon.lock` | Lock file (single instance) |
| `/var/log/matt_daemon/matt_daemon.log` | Archivo de log |

## Limitaciones

- Solo IPv4
- Sin cifrado (texto plano)
- Sin autenticación
- Máximo 3 clientes simultáneos
- Requiere root

## Estructura del Proyecto

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