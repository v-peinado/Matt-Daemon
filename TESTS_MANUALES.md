# ✅ TESTS MANUALES - MATT_DAEMON

Lista de tests fundamentales para ejecutar manualmente, tanto con **sudo** como con **Docker**.

---

## 📋 TESTS CON SUDO (LOCAL)

### Pre-requisitos
```bash
cd ~/Desktop/42cursus/Matt-Daemon/
```

---

### ✅ TEST 1: Compilación

```bash
make fclean
make
```

**Verificar:**
- [ ] Compila sin errores
- [ ] Sin warnings
- [ ] Existe el ejecutable `Matt_daemon`

```bash
ls -l Matt_daemon
```

---

### ✅ TEST 2: Requiere Root

```bash
# Intentar ejecutar SIN sudo (debe fallar)
./Matt_daemon
```

**Verificar:**
- [ ] Sale con error
- [ ] Indica que requiere privilegios de root
- [ ] No se queda corriendo

---

### ✅ TEST 3: Inicio del Daemon

```bash
# Limpiar entorno
sudo pkill -9 Matt_daemon 2>/dev/null
sudo rm -f /var/lock/matt_daemon.lock

# Iniciar daemon
sudo ./Matt_daemon
```

**Verificar en otra terminal:**

```bash
# Ver proceso
ps aux | grep Matt_daemon | grep -v grep

# Verificar PID
pgrep Matt_daemon

# Verificar TTY (debe ser "?")
ps -p $(pgrep Matt_daemon) -o tty=

# Verificar working directory (debe ser "/")
sudo pwdx $(pgrep Matt_daemon)
```

**Checklist:**
- [ ] Daemon corriendo
- [ ] TTY = `?` (sin terminal)
- [ ] Working directory = `/`
- [ ] Proceso en background

---

### ✅ TEST 3B: Verificación del Doble Fork (PPID = 1)

**Concepto:** Matt_daemon usa **doble fork()** para convertirse en un verdadero daemon. Esto hace que el proceso sea adoptado por **init/systemd (PID 1)** y quede **completamente independiente** del proceso que lo lanzó.

#### Verificación del PPID

```bash
# Ver información completa del proceso
ps -p $(pgrep Matt_daemon) -o pid,ppid,user,cmd

# Obtener solo el PPID
DAEMON_PPID=$(ps -p $(pgrep Matt_daemon) -o ppid= | tr -d ' ')
echo "PPID del daemon: $DAEMON_PPID"

# Verificar que es 1
if [ "$DAEMON_PPID" -eq 1 ]; then
    echo "✓ Daemon adoptado por init/systemd (doble fork correcto)"
else
    echo "✗ PPID=$DAEMON_PPID - No es init (error en doble fork)"
fi
```

**Checklist:**
- [ ] PPID = `1` (adoptado por init/systemd)
- [ ] PPID NO es el PID del shell que lo lanzó
- [ ] Proceso completamente huérfano e independiente

#### Árbol de Procesos

```bash
# Ver árbol de procesos
pstree -p | grep Matt_daemon

# Debe mostrar:
# systemd(1)───Matt_daemon(12345)
#
# NO debe mostrar:
# bash(2000)───Matt_daemon(12345)
```

**Verificar:**
- [ ] Padre directo es `systemd(1)` o `init(1)`
- [ ] NO hay procesos intermedios

---

#### 📚 Explicación Técnica: ¿Por qué Doble Fork?

**DOBLE FORK (Matt_daemon - Implementación Correcta):**

```
Paso 1: Proceso Original (shell/sudo)
        PID: 1000
        ↓
        fork()  ← Primer fork
        ↓
Paso 2: Primer Hijo
        PID: 1001
        PPID: 1000 (proceso original)
        ↓
        setsid()  ← Crea nueva sesión, se convierte en líder
        ↓
        fork()  ← Segundo fork (CLAVE)
        ↓
        exit()  ← El primer hijo TERMINA inmediatamente
        
Paso 3: Segundo Hijo (DAEMON FINAL)
        PID: 1002
        PPID: 1 (init/systemd lo adopta porque el padre murió)
        ✓ NO es líder de sesión (no puede adquirir terminal)
        ✓ Completamente independiente
        ✗ NO RECUPERABLE por el proceso original
```

**Resultado:**
- ✅ Daemon verdadero
- ✅ No puede adquirir terminal de control
- ✅ Sobrevive aunque el proceso original termine
- ✅ Adoptado por init (PPID=1)
- ❌ **NO recuperable** - El proceso original no puede hacer `wait()` sobre él

---

**FORK SIMPLE (NO RECOMENDADO - Solo para comparación):**

```
Paso 1: Proceso Original
        PID: 1000
        ↓
        fork()  ← Un solo fork
        ↓
        (padre sigue vivo)
        
Paso 2: Hijo
        PID: 1001
        PPID: 1000 (proceso original SIGUE VIVO)
        ⚠️ Padre puede hacer wait() para recuperarlo
        ⚠️ Si padre muere sin wait() → proceso zombie
        ⚠️ Aún vinculado a la sesión del padre
```

**Resultado:**
- ❌ NO es un daemon verdadero
- ❌ Dependiente del proceso padre
- ✅ **SÍ RECUPERABLE** - El padre puede hacer `wait()` y recuperar el estado
- ⚠️ Si el shell/padre termina, pueden ocurrir problemas

---

#### Test de No-Recuperabilidad

```bash
# Obtener PID del daemon
DAEMON_PID=$(pgrep Matt_daemon)

# Obtener PPID
DAEMON_PPID=$(ps -p $DAEMON_PID -o ppid= | tr -d ' ')

echo "Daemon PID: $DAEMON_PID"
echo "Daemon PPID: $DAEMON_PPID"

# Intentar ver el proceso padre
ps -p $DAEMON_PPID -o pid,cmd

# Debe mostrar:
#   PID CMD
#     1 /sbin/init
# O:
#     1 /lib/systemd/systemd
```

**Verificar:**
- [ ] PPID = 1 (init/systemd)
- [ ] El proceso padre original (sudo/shell) ya NO existe
- [ ] No hay forma de recuperar el daemon desde el proceso original

#### ¿Por qué esto es importante?

1. **Sin doble fork (un solo fork):**
   - El daemon podría morir si el terminal se cierra
   - El daemon podría quedar zombie si el padre termina sin hacer wait()
   - El daemon podría recibir señales de su grupo de procesos

2. **Con doble fork (Matt_daemon):**
   - El daemon es totalmente independiente
   - Sobrevive aunque el terminal/shell se cierre
   - No recibe señales del grupo de procesos original
   - Adoptado por init, que se encarga de recoger su estado cuando termine

**Checklist Final:**
- [ ] PPID = 1 confirmado
- [ ] Proceso padre original no existe
- [ ] Daemon completamente independiente
- [ ] No recuperable (comportamiento correcto)

### ✅ TEST 4: Lock File

```bash
# Verificar que existe
ls -l /var/lock/matt_daemon.lock

# Intentar segunda instancia (debe fallar)
sudo ./Matt_daemon
```

**Verificar:**
- [ ] Lock file existe
- [ ] Segunda instancia rechazada
- [ ] Mensaje: `Can't open :/var/lock/matt_daemon.lock`

```bash
# Ver en log
sudo tail -5 /var/log/matt_daemon/matt_daemon.log
```

**Verificar en log:**
- [ ] Contiene: `Error file locked.`

---

### ✅ TEST 5: Log File

```bash
# Ver directorio
ls -ld /var/log/matt_daemon/

# Ver archivo
ls -l /var/log/matt_daemon/matt_daemon.log

# Ver contenido
sudo cat /var/log/matt_daemon/matt_daemon.log
```

**Verificar:**
- [ ] Directorio `/var/log/matt_daemon/` existe
- [ ] Archivo `matt_daemon.log` existe
- [ ] Contiene entradas de log
- [ ] Primera entrada: `Started.`

---

### ✅ TEST 6: Formato de Logs

```bash
# Ver primeras líneas
sudo head -10 /var/log/matt_daemon/matt_daemon.log
```

**Verificar formato de cada línea:**
```
[08/03/2026-19:24:54] [ INFO ] - Matt_daemon: Started.
^                    ^^      ^^              ^
timestamp            espacios                nombre
```

**Checklist:**
- [ ] Timestamp: `[DD/MM/YYYY-HH:MM:SS]`
- [ ] Nivel con espacios: `[ INFO ]`, `[ LOG ]`, `[ WARNING ]`, `[ ERROR ]`
- [ ] Nombre: `Matt_daemon:`
- [ ] Mensaje después

**⚠️ IMPORTANTE:** El nivel debe tener espacios: `[ INFO ]` NO `[INFO]`

---

### ✅ TEST 7: Puerto 4242

```bash
# Verificar puerto abierto
sudo lsof -i :4242
```

**Verificar:**
- [ ] Puerto 4242 en estado LISTEN
- [ ] Proceso: Matt_daemon

**Alternativas:**
```bash
sudo netstat -tlnp | grep 4242
sudo ss -tlnp | grep 4242
```

---

### ✅ TEST 8: Conexión y Mensajes

```bash
# Conectar al daemon
nc localhost 4242
```

**Dentro de nc, escribir:**
```
test message 1
hello world
mensaje de prueba
```

**En otra terminal, verificar log:**
```bash
sudo tail -f /var/log/matt_daemon/matt_daemon.log
```

**Verificar:**
- [ ] Cada mensaje aparece en el log
- [ ] Formato: `[ LOG ] - Matt_daemon: User input: test message 1`
- [ ] Mensajes en tiempo real

**Volver a nc y escribir:**
```
quit
```

**Verificar:**
- [ ] Daemon termina
- [ ] Log contiene: `Request quit.`
- [ ] Log contiene: `Quitting.`

---

### ✅ TEST 9: Límite de 3 Clientes

```bash
# Reiniciar daemon
sudo pkill -9 Matt_daemon
sudo rm -f /var/lock/matt_daemon.lock
sudo ./Matt_daemon
```

**Abrir 4 terminales diferentes:**

**Terminal 1:**
```bash
nc localhost 4242
# NO escribir nada, dejar conectado
```

**Terminal 2:**
```bash
nc localhost 4242
# NO escribir nada, dejar conectado
```

**Terminal 3:**
```bash
nc localhost 4242
# NO escribir nada, dejar conectado
```

**Terminal 4:**
```bash
nc localhost 4242
# Este debe rechazarse o cerrarse inmediatamente
```

**Terminal 5 (verificación):**
```bash
# Ver conexiones activas
sudo lsof -i :4242 | grep ESTABLISHED
# Debe mostrar EXACTAMENTE 3 conexiones

# Contar conexiones
sudo lsof -i :4242 | grep ESTABLISHED | wc -l
# Debe ser: 3
```

**Verificar en log:**
```bash
sudo tail -10 /var/log/matt_daemon/matt_daemon.log
```

**Checklist:**
- [ ] 3 clientes conectados
- [ ] 4º cliente rechazado
- [ ] Log contiene: `Maximum clients reached, rejecting connection`

**Cerrar conexiones:** Ctrl+C en cada terminal con nc

---

### ✅ TEST 10: Señal SIGTERM

```bash
# Reiniciar daemon
sudo pkill -9 Matt_daemon
sudo rm -f /var/lock/matt_daemon.lock
sudo ./Matt_daemon
sleep 2

# Obtener PID
PID=$(pgrep Matt_daemon)
echo "PID: $PID"

# Enviar SIGTERM
sudo kill -15 $PID

# Esperar 2 segundos
sleep 2

# Verificar que terminó
pgrep Matt_daemon
# No debe devolver nada
```

**Verificar log:**
```bash
sudo tail -10 /var/log/matt_daemon/matt_daemon.log
```

**Checklist:**
- [ ] Daemon terminó
- [ ] Log contiene: `Signal handler` o `Received signal: SIGTERM`
- [ ] Log contiene: `Quitting.`

**Verificar lock file eliminado:**
```bash
ls /var/lock/matt_daemon.lock
# Debe devolver error: No such file
```

- [ ] Lock file eliminado

---

### ✅ TEST 11: Señal SIGINT

```bash
sudo ./Matt_daemon
sleep 2
sudo kill -2 $(pgrep Matt_daemon)
sleep 2
sudo tail -5 /var/log/matt_daemon/matt_daemon.log
```

**Verificar:**
- [ ] Daemon terminó
- [ ] Log registra la señal
- [ ] Lock file eliminado

---

### ✅ TEST 12: Señal SIGQUIT

```bash
sudo ./Matt_daemon
sleep 2
sudo kill -3 $(pgrep Matt_daemon)
sleep 2
sudo tail -5 /var/log/matt_daemon/matt_daemon.log
```

**Verificar:**
- [ ] Daemon terminó
- [ ] Log registra la señal
- [ ] Lock file eliminado

---

### ✅ TEST 13: Señal SIGHUP

```bash
sudo ./Matt_daemon
sleep 2
sudo kill -1 $(pgrep Matt_daemon)
sleep 2
sudo tail -5 /var/log/matt_daemon/matt_daemon.log
```

**Verificar:**
- [ ] Daemon terminó
- [ ] Log registra la señal
- [ ] Lock file eliminado

---

### ✅ TEST 14: SIGKILL (Lock File Huérfano)

```bash
sudo ./Matt_daemon
sleep 2

# SIGKILL (kill -9)
sudo kill -9 $(pgrep Matt_daemon)
sleep 1

# Verificar lock file PERSISTE
ls -l /var/lock/matt_daemon.lock
```

**Verificar:**
- [ ] Daemon muerto (killed)
- [ ] Lock file EXISTE (comportamiento esperado con SIGKILL)
- [ ] Este es el ÚNICO caso donde el lock file queda huérfano

```bash
# Limpiar manualmente
sudo rm /var/lock/matt_daemon.lock
```

---

### ✅ TEST 15: Mensajes Largos

```bash
sudo ./Matt_daemon
sleep 2

# Enviar mensaje muy largo (2000 caracteres)
python3 -c "print('X' * 2000)" | nc -w 1 localhost 4242

# Verificar daemon sigue corriendo
pgrep Matt_daemon

# Terminar
echo "quit" | nc -w 1 localhost 4242
```

**Verificar:**
- [ ] Daemon estable con mensaje largo
- [ ] No crashea
- [ ] Mensaje en log (posiblemente truncado)

---

### ✅ TEST 16: Caracteres Especiales

```bash
sudo ./Matt_daemon
sleep 2

echo "àéíóú ñ 日本語 🚀 @#$%" | nc -w 1 localhost 4242

pgrep Matt_daemon
echo "quit" | nc -w 1 localhost 4242
```

**Verificar:**
- [ ] Daemon estable
- [ ] No crashea

---

### ✅ TEST 17: Mensaje Vacío

```bash
sudo ./Matt_daemon
sleep 2

echo "" | nc -w 1 localhost 4242

pgrep Matt_daemon
echo "quit" | nc -w 1 localhost 4242
```

**Verificar:**
- [ ] Daemon estable
- [ ] No crashea

---

### ✅ TEST 18: Valgrind (Memory Leaks)

```bash
# Limpiar
sudo pkill -9 Matt_daemon
sudo rm -f /var/lock/matt_daemon.lock

# Ejecutar con Valgrind
sudo valgrind --leak-check=full --show-leak-kinds=all ./Matt_daemon
```

**En otra terminal:**
```bash
sleep 5
echo "quit" | nc -w 1 localhost 4242
```

**Verificar salida de Valgrind:**
- [ ] `All heap blocks were freed` → ✅ Perfecto
- [ ] `still reachable: X bytes` → ⚠️ Aceptable (C++ runtime)
- [ ] `definitely lost: 0 bytes` → ✅ Sin memory leaks
- [ ] `definitely lost: X bytes` → ❌ Memory leak a corregir

---

## 🐳 TESTS CON DOCKER

### Setup Docker

**Tu Dockerfile actual:**

```bash
cd ~/Desktop/42cursus/Matt-Daemon/

# Verificar que tienes este Dockerfile:
cat Dockerfile
```

**Debe contener:**
```dockerfile
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    g++ \
    make \
    netcat-openbsd \
    procps \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY Makefile .
COPY includes/ includes/
COPY src/ src/

RUN make re

EXPOSE 4242

CMD ["./Matt_daemon"]
```

**Características de este Dockerfile:**
- ✅ Ubuntu 24.04 (LTS)
- ✅ `netcat-openbsd` (versión moderna de netcat)
- ✅ Copia solo archivos necesarios (Makefile, includes, src)
- ✅ Compila automáticamente con `make re`
- ✅ CMD ejecuta el daemon directamente (como root en el contenedor)

---

### Construir Imagen

```bash
# Construir imagen Docker
docker build -t matt_daemon .

# Ver progreso de build
# Debería:
# 1. Descargar Ubuntu 24.04
# 2. Instalar dependencias
# 3. Copiar archivos
# 4. Compilar con make re
```

**Verificar:**
- [ ] Build completado sin errores
- [ ] No hay warnings de compilación

```bash
# Verificar imagen creada
docker images | grep matt_daemon

# Debe mostrar:
# matt_daemon   latest   abc123def456   X minutes ago   XXX MB
```

**Checklist:**
- [ ] Imagen construida exitosamente
- [ ] Tag: `matt_daemon:latest`

---

### ⚠️ Diferencias con Dockerfile Genérico

**Tu Dockerfile vs. Otros:**

| Aspecto | Tu Dockerfile | Otros ejemplos |
|---------|--------------|----------------|
| CMD | `["./Matt_daemon"]` | `["/bin/bash"]` |
| netcat | `netcat-openbsd` | `netcat` |
| Build | Automático (`make re`) | Manual |

**Implicaciones:**

1. **CMD ejecuta el daemon automáticamente:**
   - Con `docker run matt_daemon` el daemon se inicia solo
   - Para shell interactivo: `docker run -it matt_daemon /bin/bash`

2. **netcat-openbsd:**
   - Usa `nc` (igual que netcat normal)
   - Versión más moderna y mantenida

---

### ✅ TEST D1: Inicio en Docker (Modo Interactivo)

**Opción 1: Shell Interactivo (RECOMENDADO para testing)**

```bash
# Override CMD para obtener bash interactivo
docker run -it --rm --name matt_test -p 4242:4242 matt_daemon /bin/bash
```

**⚠️ IMPORTANTE:** Tu Dockerfile tiene `CMD ["./Matt_daemon"]`, así que por defecto ejecuta el daemon automáticamente. Para hacer testing manual, necesitas especificar `/bin/bash` para overridear el CMD.

**Dentro del contenedor (ya eres root):**

```bash
# Verificar ejecutable
ls -l Matt_daemon

# Iniciar daemon manualmente (en background)
./Matt_daemon &

# Verificar proceso
ps aux | grep Matt_daemon

# Verificar PID, PPID y TTY
ps -p $(pgrep Matt_daemon) -o pid,ppid,tty,cmd

# Verificar PPID = 1 (doble fork correcto)
DAEMON_PPID=$(ps -p $(pgrep Matt_daemon) -o ppid= | tr -d ' ')
echo "PPID del daemon: $DAEMON_PPID"

# Verificar working directory
pwdx $(pgrep Matt_daemon)
```

**Checklist:**
- [ ] Daemon corriendo
- [ ] PPID = 1 (adoptado por init)
- [ ] TTY = ? (sin terminal)
- [ ] PWD = /

---

**Opción 2: Daemon Automático (Como está configurado)**

```bash
# El daemon se ejecuta automáticamente
docker run -d --name matt_daemon_auto -p 4242:4242 matt_daemon

# Ver logs del contenedor
docker logs matt_daemon_auto

# Entrar al contenedor mientras el daemon corre
docker exec -it matt_daemon_auto /bin/bash

# Dentro, verificar proceso
ps aux | grep Matt_daemon
```

**⚠️ Nota:** En este modo, el daemon es el proceso principal (PID 1) del contenedor, NO tendrá PPID=1 porque ES el PID 1.

```bash
# Ver proceso
ps -p $(pgrep Matt_daemon) -o pid,ppid,cmd

# En contenedor, el daemon podría ser PID 1
# Esto es normal en Docker cuando es el CMD principal
```

**Limpiar:**
```bash
docker stop matt_daemon_auto
docker rm matt_daemon_auto
```

---

### 📝 Nota sobre PPID en Docker

**Comportamiento de PPID según cómo ejecutes el contenedor:**

| Modo | Comando | PPID del daemon | Explicación |
|------|---------|-----------------|-------------|
| **Interactivo con bash** | `docker run -it matt_daemon /bin/bash` | PPID = 1 | Daemon hace doble fork, adoptado por init del contenedor |
| **Automático (CMD)** | `docker run -d matt_daemon` | Daemon ES PID 1 | Daemon es el proceso principal del contenedor |

**¿Por qué el daemon puede ser PID 1 en Docker?**

Cuando ejecutas `docker run matt_daemon` (sin override), el Dockerfile ejecuta `CMD ["./Matt_daemon"]`, lo que significa que **el daemon es el primer proceso del contenedor**.

```bash
# En este caso:
docker run -d matt_daemon
docker exec matt_daemon ps aux

# Verás:
# PID TTY   CMD
#   1 ?     ./Matt_daemon  ← El daemon ES PID 1
```

Esto es **normal en Docker** y **no es un error**. El doble fork sigue funcionando correctamente, pero dentro del namespace del contenedor, no hay un "init" real esperando adoptar procesos.

**Para testing más realista del doble fork:**
- Usa modo interactivo: `docker run -it matt_daemon /bin/bash`
- Luego ejecuta manualmente: `./Matt_daemon &`
- Así verás PPID=1 (el bash es PID 1, y el daemon es adoptado)

---

```bash
# Dentro del contenedor
lsof -i :4242
```

**Verificar:**
- [ ] Puerto 4242 abierto
- [ ] Proceso Matt_daemon

---

### ✅ TEST D3: Logs en Docker

```bash
# Dentro del contenedor
cat /var/log/matt_daemon/matt_daemon.log

# Ver en tiempo real
tail -f /var/log/matt_daemon/matt_daemon.log
```

**Verificar:**
- [ ] Log file existe
- [ ] Formato correcto
- [ ] Contiene `Started.`

---

### ✅ TEST D4: Conexión en Docker

```bash
# Dentro del contenedor
nc localhost 4242
```

**Escribir:**
```
test desde docker
hello from container
mensaje de prueba
quit
```

**Verificar en log:**
```bash
cat /var/log/matt_daemon/matt_daemon.log
```

**Checklist:**
- [ ] Mensajes recibidos
- [ ] Logueados correctamente
- [ ] Daemon terminó con quit

---

### ✅ TEST D5: Lock File en Docker

```bash
# Dentro del contenedor
./Matt_daemon &
sleep 2

# Ver lock file
ls -l /var/lock/matt_daemon.lock

# Intentar segunda instancia
./Matt_daemon

# Ver log
tail -5 /var/log/matt_daemon/matt_daemon.log
```

**Verificar:**
- [ ] Lock file existe
- [ ] Segunda instancia rechazada
- [ ] Error en log

---

### ✅ TEST D6: Múltiples Clientes en Docker

```bash
# Dentro del contenedor
./Matt_daemon &
sleep 2

# Abrir conexiones en background
nc localhost 4242 &
nc localhost 4242 &
nc localhost 4242 &

# Verificar conexiones
lsof -i :4242 | grep ESTABLISHED

# Intentar 4ta (debe fallar)
nc localhost 4242
```

**Verificar:**
- [ ] 3 conexiones activas
- [ ] 4ta rechazada

```bash
# Limpiar
killall nc
echo "quit" | nc -w 1 localhost 4242
```

---

### ✅ TEST D7: Señales en Docker

```bash
# Dentro del contenedor
./Matt_daemon &
sleep 2

# SIGTERM
kill -15 $(pgrep Matt_daemon)
sleep 2
tail -5 /var/log/matt_daemon/matt_daemon.log

# Verificar terminó
pgrep Matt_daemon
# No debe devolver nada
```

**Verificar:**
- [ ] Daemon terminó
- [ ] Señal registrada en log
- [ ] Lock file eliminado

---

### ✅ TEST D8: Conexión desde el Host

**Terminal 1 (Docker):**
```bash
docker run -d --name matt_daemon_test -p 4242:4242 matt_daemon \
  /bin/bash -c "./Matt_daemon && tail -f /dev/null"
```

**Terminal 2 (Host):**
```bash
# Conectar desde tu máquina local
nc localhost 4242
```

**Escribir:**
```
mensaje desde el host
test desde fuera del contenedor
quit
```

**Verificar logs desde host:**
```bash
docker exec matt_daemon_test cat /var/log/matt_daemon/matt_daemon.log
```

**Limpiar:**
```bash
docker stop matt_daemon_test
docker rm matt_daemon_test
```

**Checklist:**
- [ ] Conexión exitosa desde host
- [ ] Mensajes logueados
- [ ] Daemon termina con quit

---

## 📚 CONCEPTOS CLAVE

### Doble Fork - Resumen

| Aspecto | Doble Fork (Matt_daemon) | Fork Simple |
|---------|-------------------------|-------------|
| **PPID** | = 1 (adoptado por init) | = PID del padre |
| **Recuperable** | ❌ NO | ✅ SÍ (con wait()) |
| **Terminal** | ❌ No puede adquirir | ⚠️ Puede adquirir |
| **Supervivencia** | ✅ Sobrevive si padre muere | ⚠️ Puede convertirse en zombie |
| **Verdadero daemon** | ✅ SÍ | ❌ NO |

**¿Por qué es importante?**
- Con doble fork: El daemon es **completamente independiente**
- Con un fork: El daemon **depende del proceso padre**

**Verificación:**
```bash
# PPID debe ser 1
ps -p $(pgrep Matt_daemon) -o ppid=
```

### Diferencias: Sudo Local vs Docker

| Aspecto | Sudo Local | Docker (Interactivo) | Docker (Automático) |
|---------|------------|---------------------|-------------------|
| **Usuario** | root (via sudo) | root (nativo) | root (nativo) |
| **PPID del daemon** | 1 (systemd/init) | 1 (bash en contenedor) | Daemon ES PID 1 |
| **Puerto 4242** | Host real | Mapeado (-p 4242:4242) | Mapeado |
| **Logs** | `/var/log/matt_daemon/` | `/var/log/matt_daemon/` | `/var/log/matt_daemon/` |
| **Lock file** | `/var/lock/` | `/var/lock/` | `/var/lock/` |

### Tu Dockerfile Específico

**Características:**
- Ubuntu 24.04 (LTS más reciente)
- `netcat-openbsd` (versión moderna de nc)
- Compila automáticamente durante build
- CMD ejecuta daemon directamente

**Para testing interactivo:**
```bash
# Override CMD
docker run -it --rm -p 4242:4242 matt_daemon /bin/bash
```

**Para ejecución automática:**
```bash
# Usa CMD por defecto
docker run -d -p 4242:4242 matt_daemon
```

---

## 📊 RESUMEN DE VERIFICACIÓN

### Sudo Local
- [ ] ✅ Compilación limpia
- [ ] ✅ Solo funciona con sudo
- [ ] ✅ Daemon en background (TTY=?)
- [ ] ✅ **PPID = 1** (doble fork correcto, adoptado por init/systemd)
- [ ] ✅ Working directory = /
- [ ] ✅ Lock file creado
- [ ] ✅ Log file creado
- [ ] ✅ Formato de logs CORRECTO: `[ INFO ]` con espacios
- [ ] ✅ Puerto 4242 abierto
- [ ] ✅ Acepta conexiones
- [ ] ✅ Máximo 3 clientes
- [ ] ✅ Comando quit funciona
- [ ] ✅ Todas las señales manejadas (15, 2, 3, 1)
- [ ] ✅ SIGKILL deja lock huérfano (esperado)
- [ ] ✅ Sin memory leaks (Valgrind)

### Docker
- [ ] ✅ Imagen construye correctamente
- [ ] ✅ Daemon inicia en contenedor
- [ ] ✅ PPID correcto (1 en modo interactivo, o daemon ES PID 1 en modo automático)
- [ ] ✅ Puerto accesible desde host
- [ ] ✅ Logs funcionan
- [ ] ✅ Lock file funciona
- [ ] ✅ Todo igual que en sudo local

---

## 🎯 CHECKLIST PRE-EVALUACIÓN

**Ejecutar estos 6 tests antes de ir a evaluación:**

### 1. Compilación
```bash
make fclean && make
```
- [ ] Sin errores ni warnings

### 2. Test Básico + PPID
```bash
sudo ./Matt_daemon &
ps aux | grep Matt_daemon
sudo lsof -i :4242

# VERIFICAR PPID = 1 (IMPORTANTE)
ps -p $(pgrep Matt_daemon) -o ppid=
```
- [ ] Daemon corriendo
- [ ] Puerto abierto
- [ ] **PPID = 1** (doble fork correcto)

### 3. Formato de Logs (CRÍTICO)
```bash
sudo head -5 /var/log/matt_daemon/matt_daemon.log
```
- [ ] **DEBE VER:** `[ INFO ]` con espacios
- [ ] **NO DEBE VER:** `[INFO]` sin espacios

### 4. Test de 3 Clientes
```bash
# 3 terminales con nc, 4ta debe fallar
```

### 5. Comando Quit
```bash
echo "quit" | nc localhost 4242
sudo tail -5 /var/log/matt_daemon/matt_daemon.log
```
- [ ] Daemon termina
- [ ] Log registra `Quitting.`
- [ ] Lock file eliminado

### 6. Docker (Opcional)
```bash
docker build -t matt_daemon .
docker run -it --rm -p 4242:4242 matt_daemon /bin/bash
# Dentro: ./Matt_daemon &
```
- [ ] Build exitoso
- [ ] Daemon funciona igual que en local

---

**¡Listo para evaluación!** 🚀
