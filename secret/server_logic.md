# Server - Flujo de Ejecución

## Tabla de Contenidos

1. [Overview](#overview)
2. [Arquitectura](#arquitectura)
3. [Fase 1: Inicialización](#fase-1-inicialización)
4. [Fase 2: Main Loop](#fase-2-main-loop)
5. [Select y Multiplexación](#select-y-multiplexación)
6. [Manejo de Señales](#manejo-de-señales)
7. [Procesamiento de Mensajes](#procesamiento-de-mensajes)
8. [Gestión de Múltiples Clientes](#gestión-de-múltiples-clientes)
9. [Estados del Servidor](#estados-del-servidor)
10. [Ejemplo Completo](#ejemplo-completo)

---

## Overview

`Server` es responsable de:

- ✅ Escuchar en puerto 4242
- ✅ Aceptar hasta 3 clientes simultáneos
- ✅ Multiplexar I/O con `select()`
- ✅ Recibir mensajes de clientes
- ✅ Procesar comando "quit" → shutdown
- ✅ Loguear otros mensajes
- ✅ Manejar señales del sistema (SIGTERM, SIGINT, SIGQUIT, SIGHUP)

---

## Arquitectura
```
┌─────────────────────────────────────────────────────────────┐
│                         SERVER                              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  MIEMBROS:                                                  │
│  ├─ m_server_fd     : Socket TCP (puerto 4242)            │
│  ├─ m_signal_fd     : signalfd (señales del sistema)      │
│  ├─ m_client_fds    : vector<int> (hasta 3 clientes)      │
│  ├─ m_read_fds      : fd_set (para select())              │
│  ├─ m_logger        : TintinReporter&                      │
│  └─ m_running       : bool (estado del loop)               │
│                                                             │
│  FLUJO:                                                     │
│  1. init()          → Setup sockets y señales              │
│  2. run()           → Main loop (select)                   │
│  3. stop()          → Shutdown limpio                      │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## Fase 1: Inicialización

### Secuencia de `init()`
```cpp
bool Server::init()
{
    createSocket();     // 1. Crear socket TCP
    bindSocket();       // 2. Bind a 0.0.0.0:4242
    listenSocket();     // 3. Listen (backlog 5)
    setupSignals();     // 4. Configurar signalfd
    return true;
}
```

### Detalle de cada paso:

#### 1. createSocket()
```cpp
m_server_fd = socket(AF_INET, SOCK_STREAM, 0);
setsockopt(m_server_fd, SOL_SOCKET, SO_REUSEADDR, ...);
```

**Resultado:**
- `m_server_fd = 3` (ejemplo)
- Socket TCP creado
- SO_REUSEADDR activado (evita "Address already in use")

#### 2. bindSocket()
```cpp
struct sockaddr_in addr;
addr.sin_family = AF_INET;
addr.sin_addr.s_addr = INADDR_ANY;  // 0.0.0.0
addr.sin_port = htons(4242);
bind(m_server_fd, &addr, sizeof(addr));
```

**Resultado:**
- Socket vinculado a `0.0.0.0:4242`
- Listo para aceptar conexiones

#### 3. listenSocket()
```cpp
listen(m_server_fd, 5);  // backlog = 5
```

**Resultado:**
- Socket en modo listening
- Puede encolar hasta 5 conexiones pendientes

#### 4. setupSignals()
```cpp
sigset_t mask;
sigemptyset(&mask);
sigaddset(&mask, SIGTERM);
sigaddset(&mask, SIGINT);
sigaddset(&mask, SIGQUIT);
sigaddset(&mask, SIGHUP);

sigprocmask(SIG_BLOCK, &mask, NULL);  // Bloquear entrega tradicional
m_signal_fd = signalfd(-1, &mask, SFD_NONBLOCK);
```

**Resultado:**
- `m_signal_fd = 4` (ejemplo)
- Señales convertidas en eventos de file descriptor
- Manejo sincrónico (sin race conditions)

**Estado después de `init()`:**
```
m_server_fd  = 3  (listening on 4242)
m_signal_fd  = 4  (monitoring SIGTERM/SIGINT/SIGQUIT/SIGHUP)
m_client_fds = [] (vacío)
m_running    = false
```

---

## Fase 2: Main Loop

### Flujo General
```
┌─────────────────────────────────────────────────────────────┐
│                      while (m_running)                      │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. setupFdSet()                                            │
│     └─ Preparar fd_set con server, signal, clients        │
│                                                             │
│  2. select()                                                │
│     └─ BLOQUEA hasta: evento o timeout (1s)               │
│                                                             │
│  3. Procesar eventos por PRIORIDAD:                        │
│     ┌─────────────────────────────────────┐               │
│     │ A) ¿Señal? → handleSignal()         │               │
│     │              → shutdown              │               │
│     └─────────────────────────────────────┘               │
│     ┌─────────────────────────────────────┐               │
│     │ B) ¿Nueva conexión?                 │               │
│     │    → acceptNewClient()               │               │
│     └─────────────────────────────────────┘               │
│     ┌─────────────────────────────────────┐               │
│     │ C) ¿Datos de clientes?              │               │
│     │    → handleClientData(fd)           │               │
│     └─────────────────────────────────────┘               │
│                                                             │
│  4. Repetir                                                 │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Código del Loop
```cpp
void Server::run()
{
    m_running = true;
    
    while (m_running)
    {
        setupFdSet();
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int activity = select(getMaxFd() + 1, &m_read_fds, NULL, NULL, &timeout);
        
        if (activity < 0 && errno != EINTR)
            break;
        
        if (activity == 0)
            continue;
        
        // Prioridad 1: Señales
        if (FD_ISSET(m_signal_fd, &m_read_fds))
        {
            handleSignal();
            continue;
        }
        
        // Prioridad 2: Nuevas conexiones
        if (FD_ISSET(m_server_fd, &m_read_fds))
        {
            acceptNewClient();
        }
        
        // Prioridad 3: Datos de clientes
        for (size_t i = 0; i < m_client_fds.size(); )
        {
            int clientFd = m_client_fds[i];
            if (FD_ISSET(clientFd, &m_read_fds))
                handleClientData(clientFd);
            
            if (i < m_client_fds.size() && m_client_fds[i] == clientFd)
                i++;
        }
    }
}
```

---

## Select y Multiplexación

### ¿Cómo funciona `select()`?

#### Antes de `select()`:
```
fd_set m_read_fds
┌─────────────────────────────────────────┐
│ fd 3 (m_server_fd)    : [X] monitoreado│
│ fd 4 (m_signal_fd)    : [X] monitoreado│
│ fd 5 (cliente 1)      : [X] monitoreado│
│ fd 6 (cliente 2)      : [X] monitoreado│
│ fd 7 (cliente 3)      : [X] monitoreado│
└─────────────────────────────────────────┘

select() BLOQUEA esperando actividad...
```

#### Durante `select()`:
```
⏳ Kernel monitorea todos los fds
⏳ Esperando que alguno tenga datos listos...
⏳ Timeout: 1 segundo
```

#### Después de `select()` (evento: cliente 1 envía datos):
```
fd_set m_read_fds (modificado por select)
┌─────────────────────────────────────────┐
│ fd 3 (m_server_fd)    : [ ] sin actividad  │
│ fd 4 (m_signal_fd)    : [ ] sin actividad  │
│ fd 5 (cliente 1)      : [X] ¡HAY DATOS!    │
│ fd 6 (cliente 2)      : [ ] sin actividad  │
│ fd 7 (cliente 3)      : [ ] sin actividad  │
└─────────────────────────────────────────┘

select() retorna 1 (un fd tiene actividad)
→ FD_ISSET(5, &m_read_fds) es true
→ handleClientData(5)
```

### setupFdSet()
```cpp
void Server::setupFdSet()
{
    FD_ZERO(&m_read_fds);                    // Limpiar el set
    FD_SET(m_server_fd, &m_read_fds);        // Añadir server socket
    
    if (m_signal_fd >= 0)
        FD_SET(m_signal_fd, &m_read_fds);    // Añadir signal fd
    
    for (size_t i = 0; i < m_client_fds.size(); i++)
        FD_SET(m_client_fds[i], &m_read_fds); // Añadir todos los clientes
}
```

### getMaxFd()
```cpp
int Server::getMaxFd() const
{
    int maxFd = m_server_fd;
    
    if (m_signal_fd > maxFd)
        maxFd = m_signal_fd;
    
    for (size_t i = 0; i < m_client_fds.size(); i++)
        if (m_client_fds[i] > maxFd)
            maxFd = m_client_fds[i];
    
    return maxFd;
}
```

**¿Por qué necesitamos `maxFd`?**

`select()` requiere `maxFd + 1` para saber cuántos descriptores revisar.

---

## Manejo de Señales

### ¿Por qué signalfd() en lugar de signal()?

#### Enfoque Tradicional (signal) - ASÍNCRONO ❌
```cpp
// Handler interrumpe el código en cualquier momento
void signalHandler(int sig)
{
    g_running = false;  // Race condition posible
}

signal(SIGTERM, signalHandler);

// Código principal
while (g_running)  // ← Puede ser interrumpido en cualquier momento
{
    // Procesando datos...
    // INTERRUMPIDO POR SEÑAL
    // Posible corrupción de datos
}
```

**Problemas:**
- Race conditions
- Solo funciones async-signal-safe
- Difícil debuggear
- Comportamiento impredecible

#### Enfoque Moderno (signalfd) - SÍNCRONO ✅
```cpp
// Señales → eventos de file descriptor
int signal_fd = signalfd(...);

while (m_running)
{
    select(..., signal_fd, ...);  // Señal detectada aquí
    
    if (FD_ISSET(signal_fd, &readfds))  // Procesamos cuando queremos
    {
        handleSignal();  // Sin interrupciones, sin race conditions
    }
}
```

**Ventajas:**
- Sin race conditions
- Cualquier función permitida
- Integrado en select() loop
- Código limpio y predecible

### Flujo Completo de una Señal
```
┌────────────────────────────────────────────────────────────┐
│ T0: Server ejecutando select()                             │
│     Esperando eventos...                                   │
└────────────────────────────────────────────────────────────┘
                            ↓
┌────────────────────────────────────────────────────────────┐
│ T1: Usuario ejecuta: kill -TERM <pid>                      │
└────────────────────────────────────────────────────────────┘
                            ↓
┌────────────────────────────────────────────────────────────┐
│ T2: Kernel procesa SIGTERM                                 │
│     - Bloqueada por sigprocmask(SIG_BLOCK)                 │
│     - Redirigida a signalfd                                │
│     - m_signal_fd (fd 4) se vuelve "readable"             │
└────────────────────────────────────────────────────────────┘
                            ↓
┌────────────────────────────────────────────────────────────┐
│ T3: select() detecta actividad en fd 4                     │
│     select() retorna 1                                     │
└────────────────────────────────────────────────────────────┘
                            ↓
┌────────────────────────────────────────────────────────────┐
│ T4: FD_ISSET(m_signal_fd, &m_read_fds) → true             │
│     handleSignal() llamado                                 │
└────────────────────────────────────────────────────────────┘
                            ↓
┌────────────────────────────────────────────────────────────┐
│ T5: handleSignal() lee la señal                            │
│     read(m_signal_fd, &siginfo, sizeof(siginfo))          │
│     siginfo.ssi_signo = 15 (SIGTERM)                      │
└────────────────────────────────────────────────────────────┘
                            ↓
┌────────────────────────────────────────────────────────────┐
│ T6: Identificar señal                                      │
│     getSignalName(15) → "SIGTERM"                          │
│     log(INFO, "Received signal: SIGTERM")                  │
│     log(INFO, "Shutting down...")                          │
└────────────────────────────────────────────────────────────┘
                            ↓
┌────────────────────────────────────────────────────────────┐
│ T7: stop()                                                 │
│     - m_running = false                                    │
│     - Cerrar todos los clientes                            │
│     - log(INFO, "All clients disconnected")                │
└────────────────────────────────────────────────────────────┘
                            ↓
┌────────────────────────────────────────────────────────────┐
│ T8: Loop termina                                           │
│     while (m_running) // false → sale del loop             │
└────────────────────────────────────────────────────────────┘
                            ↓
┌────────────────────────────────────────────────────────────┐
│ T9: Destructor                                             │
│     ~Server() cierra m_signal_fd y m_server_fd             │
└────────────────────────────────────────────────────────────┘
```

---

## Procesamiento de Mensajes

### Escenario 1: Cliente envía "hello"
```
┌────────────────────────────────────────────────────────────┐
│ Cliente: echo "hello" | nc localhost 4242                  │
└────────────────────────────────────────────────────────────┘
                            ↓
┌────────────────────────────────────────────────────────────┐
│ select() detecta actividad en client_fd (fd 5)             │
└────────────────────────────────────────────────────────────┘
                            ↓
┌────────────────────────────────────────────────────────────┐
│ handleClientData(5)                                        │
│  ├─ recv(5, buffer, 1024) → 6 bytes                       │
│  ├─ buffer = "hello\n"                                     │
│  ├─ message = "hello" (eliminado '\n')                    │
│  └─ processMessage(5, "hello")                            │
│      └─ if (message == "quit") → false                    │
│      └─ else → log(LOG, "User input: hello")              │
└────────────────────────────────────────────────────────────┘
                            ↓
┌────────────────────────────────────────────────────────────┐
│ Log file:                                                  │
│ [29/01/2026-15:30:45] [ LOG ] - Matt_daemon: User input: hello │
└────────────────────────────────────────────────────────────┘
```

### Escenario 2: Cliente envía "quit"
```
┌────────────────────────────────────────────────────────────┐
│ Cliente: echo "quit" | nc localhost 4242                   │
└────────────────────────────────────────────────────────────┘
                            ↓
┌────────────────────────────────────────────────────────────┐
│ handleClientData(client_fd)                                │
│  ├─ recv() → "quit\n"                                      │
│  ├─ message = "quit"                                       │
│  └─ processMessage(client_fd, "quit")                     │
│      └─ if (message == "quit") → true!                    │
│          ├─ log(INFO, "Request quit.")                    │
│          └─ stop()                                         │
│              └─ m_running = false                          │
└────────────────────────────────────────────────────────────┘
                            ↓
┌────────────────────────────────────────────────────────────┐
│ Loop termina → Server shutdown                             │
│                                                            │
│ Log file:                                                  │
│ [29/01/2026-15:30:50] [ INFO ] - Matt_daemon: Request quit.    │
│ [29/01/2026-15:30:50] [ INFO ] - Matt_daemon: All clients disconnected │
│ [29/01/2026-15:30:50] [ INFO ] - Matt_daemon: Server stopped │
└────────────────────────────────────────────────────────────┘
```

### Escenario 3: Cliente se desconecta (EOF)
```
┌────────────────────────────────────────────────────────────┐
│ Cliente: nc localhost 4242                                 │
│          (presiona Ctrl+D)                                 │
└────────────────────────────────────────────────────────────┘
                            ↓
┌────────────────────────────────────────────────────────────┐
│ handleClientData(client_fd)                                │
│  ├─ recv(client_fd, buffer, 1024)                         │
│  ├─ bytesRead = 0  ← EOF (conexión cerrada)               │
│  ├─ if (bytesRead <= 0) → true                            │
│  │   └─ log(INFO, "Client disconnected")                  │
│  └─ disconnectClient(client_fd)                           │
│      ├─ close(client_fd)                                   │
│      ├─ m_client_fds.erase(...)                           │
│      └─ log(INFO, "Client removed from list")             │
└────────────────────────────────────────────────────────────┘
                            ↓
┌────────────────────────────────────────────────────────────┐
│ Estado actualizado:                                        │
│ m_client_fds antes:  [5, 6, 7]                            │
│ m_client_fds después: [5, 7]  (fd 6 eliminado)            │
└────────────────────────────────────────────────────────────┘
```

---

## Gestión de Múltiples Clientes

### Límite de 3 Clientes
```cpp
bool Server::canAcceptClient() const
{
    return m_client_fds.size() < Config::MAX_CLIENTS;  // MAX_CLIENTS = 3
}
```

### Timeline con Múltiples Clientes
```
T0: Server listening
    m_client_fds = []

T1: Cliente A conecta (127.0.0.1:54321)
    accept() → fd 5
    m_client_fds = [5]
    log(INFO, "Client connected from 127.0.0.1:54321")

T2: Cliente B conecta (127.0.0.1:54322)
    accept() → fd 6
    m_client_fds = [5, 6]
    log(INFO, "Client connected from 127.0.0.1:54322")

T3: Cliente C conecta (127.0.0.1:54323)
    accept() → fd 7
    m_client_fds = [5, 6, 7]
    log(INFO, "Client connected from 127.0.0.1:54323")

T4: Cliente D intenta conectar (4º cliente - RECHAZADO)
    canAcceptClient() → false (size == 3)
    accept() → fd 8
    close(8) inmediatamente
    log(WARNING, "Maximum clients reached, rejecting connection")

T5: select() monitorea: {3, 4, 5, 6, 7}
    - fd 3: m_server_fd (nuevas conexiones)
    - fd 4: m_signal_fd (señales)
    - fd 5: cliente A
    - fd 6: cliente B
    - fd 7: cliente C

T6: Cliente A envía "test"
    select() detecta fd 5 activo
    handleClientData(5)
    log(LOG, "User input: test")

T7: Cliente B envía "hello"
    select() detecta fd 6 activo
    handleClientData(6)
    log(LOG, "User input: hello")

T8: Cliente A se desconecta
    recv(5) → 0 (EOF)
    disconnectClient(5)
    m_client_fds = [6, 7]
    log(INFO, "Client disconnected")

T9: Ahora Cliente D puede conectar
    canAcceptClient() → true (size == 2)
    accept() → fd 8
    m_client_fds = [6, 7, 8]
    log(INFO, "Client connected")

T10: Cliente B envía "quit"
     processMessage() → stop()
     m_running = false
     Todos los clientes desconectados
     Loop termina
```

---

## Estados del Servidor
```
┌──────────────┐
│  CREATED     │  Constructor ejecutado
│              │  - m_server_fd = -1
│              │  - m_signal_fd = -1
└──────┬───────┘  - m_running = false
       │
       │ init()
       ↓
┌──────────────┐
│ INITIALIZED  │  Sockets configurados
│              │  - m_server_fd escuchando en 4242
│              │  - m_signal_fd monitoreando señales
└──────┬───────┘  - Listo para run()
       │
       │ run()
       ↓
┌──────────────┐
│   RUNNING    │  Loop select() activo
│              │  - Aceptando clientes (max 3)
│              │  - Procesando mensajes
│              │  - Monitoreando señales
└──────┬───────┘  - m_running = true
       │
       │ stop() / señal / "quit"
       ↓
┌──────────────┐
│  STOPPING    │  Cerrando recursos
│              │  - Desconectando clientes
│              │  - m_running = false
└──────┬───────┘  - Loop termina
       │
       │ ~Server()
       ↓
┌──────────────┐
│  DESTROYED   │  Recursos liberados
│              │  - Todos los fds cerrados
└──────────────┘
```

---

## Ejemplo Completo

### Una Iteración Completa del Loop
```cpp
// Estado inicial:
m_server_fd = 3
m_signal_fd = 4
m_client_fds = [5, 6]  // 2 clientes conectados
m_running = true

// ==========================================
// INICIO ITERACIÓN N
// ==========================================

// Paso 1: Preparar fd_set
setupFdSet();
// m_read_fds ahora monitorea: {3, 4, 5, 6}

// Paso 2: Calcular maxfd
int maxfd = getMaxFd();  // maxfd = 6

// Paso 3: Configurar timeout
struct timeval timeout = {1, 0};  // 1 segundo

// Paso 4: select() - BLOQUEAR
int activity = select(7, &m_read_fds, NULL, NULL, &timeout);
// ⏳ BLOQUEADO esperando eventos...

// --- 800ms después ---
// Cliente en fd 5 envía "hello\n"

// Paso 5: select() retorna
// activity = 1 (un fd tiene datos)
// m_read_fds ahora solo marca fd 5

// Paso 6: Verificar señales
if (FD_ISSET(4, &m_read_fds))  // false
{
    // No hay señales
}

// Paso 7: Verificar nuevas conexiones
if (FD_ISSET(3, &m_read_fds))  // false
{
    // Nadie está conectando
}

// Paso 8: Verificar clientes
for (size_t i = 0; i < 2; )
{
    // Iteración 1: i=0
    int clientFd = m_client_fds[0];  // fd 5
    
    if (FD_ISSET(5, &m_read_fds))  // true!
    {
        handleClientData(5);
        // ├─ recv(5, buffer, 1024) → "hello\n"
        // ├─ message = "hello"
        // └─ processMessage(5, "hello")
        //    └─ log(LOG, "User input: hello")
    }
    i++;
    
    // Iteración 2: i=1
    clientFd = m_client_fds[1];  // fd 6
    
    if (FD_ISSET(6, &m_read_fds))  // false
    {
        // Cliente 6 no tiene datos
    }
    i++;
}

// ==========================================
// FIN ITERACIÓN N
// Volver al inicio del while
// ==========================================
```

---

## Resumen

### Inicialización
```
1. socket()    → Crear socket TCP
2. bind()      → Vincular a puerto 4242
3. listen()    → Aceptar conexiones (backlog 5)
4. signalfd()  → Configurar manejo de señales
```

### Main Loop
```
while (m_running)
{
    1. setupFdSet()  → Preparar fds para select
    2. select()      → Esperar eventos (timeout 1s)
    3. Procesar por prioridad:
       a) Señales        → shutdown
       b) Conexiones     → accept (max 3)
       c) Datos clientes → recv y procesar
}
```

### Procesamiento de Mensajes
```
- "quit"  → log(INFO, "Request quit.") + stop()
- Otros   → log(LOG, "User input: <mensaje>")
```

### Manejo de Señales
```
SIGTERM/SIGINT/SIGQUIT/SIGHUP
    ↓
signalfd (fd readable)
    ↓
select() detecta
    ↓
handleSignal()
    ↓
log(INFO, "Received signal: <nombre>")
    ↓
stop() → shutdown limpio
```

### Shutdown
```
stop()
├─ m_running = false
├─ Cerrar todos los clientes
├─ log(INFO, "All clients disconnected")
└─ Loop termina

~Server()
├─ Cerrar m_signal_fd
├─ Cerrar m_server_fd
└─ Recursos liberados
```

---
