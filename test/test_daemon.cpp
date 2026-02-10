/**
 * Crea un proceso simple y lo convierte en daemon
 * 
 * g++ test/test_daemon.cpp src/TintinReporter.cpp -o test_daemon -I./includes
 * 
 * ./test_daemon
 * 
 * COMPROBAR QUE ESTÁ DAEMONIZADO:
 *   ps aux | grep test_daemon
 *   - Verás el proceso corriendo
 *   - El PPID (Parent Process ID) será 1 (init/systemd)
 *   - No tendrá terminal asociada (? en la columna TTY)
 * 
 * TERMINAR EL DAEMON:
 *   kill -15 <PID>
 *   (usa el PID que ves en ps aux)
 */

#include "TintinReporter.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>

void daemonize()
{
    // Paso 1: Hacer fork para crear proceso hijo
    pid_t pid = fork();
    
    if (pid < 0) {
        std::cerr << "Error en fork()" << std::endl;
        exit(1);
    }
    
    if (pid > 0) {
        // Padre termina, hijo continúa
        std::cout << "Proceso hijo creado con PID: " << pid << std::endl;
        std::cout << "Padre terminando..." << std::endl;
        exit(0);
    }
    
    // Paso 2: Crear nueva sesión (el hijo se convierte en líder de sesión)
    if (setsid() < 0) {
        exit(1);
    }
    
    // Paso 3: Cambiar directorio a raíz
    chdir("/");
    
    // Paso 4: Cerrar descriptores de archivo estándar
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    // Ahora somos un daemon!
}

int main()
{
    std::cout << "=== Test Daemon ===" << std::endl;
    std::cout << "PID antes de daemonizar: " << getpid() << std::endl;
    std::cout << "\nDaemonizando proceso..." << std::endl;
    std::cout << "(El proceso quedará corriendo en background)" << std::endl;
    
    // Daemonizar
    daemonize();
    
    // ✅ Ahora AQUÍ guarda el PID correcto (del hijo)
    try {
        TintinReporter reporter("test_demonize.log");
        reporter.log(TintinReporter::LogLevel::Info, "Daemon iniciado. PID: " + std::to_string(getpid()));
        
        // Guardar PID real
        std::ofstream pidfile_daemon("test_daemon.pid");
        pidfile_daemon << getpid();  // ← Ahora sí, el PID correcto
        pidfile_daemon.close();
        
        // Loop...
    } catch (std::exception& e) {
        // Si falla, al menos sabemos por qué
        std::ofstream errfile("test_daemon_error.log");
        errfile << "Error: " << e.what() << std::endl;
        errfile.close();
        return 1;
    }
}

/**
 * CÓMO COMPROBAR QUE ESTÁ DAEMONIZADO:
 * =====================================
 * 
 * 1. Ejecuta el programa:
 *    ./test_daemon
 * 
 * 2. Verás un mensaje y el proceso se irá a background
 * 
 * 3. Comprueba que está corriendo:
 *    ps aux | grep test_daemon
 * 
 *    Verás algo como:
 *    root  1234  0.0  0.0  15172  2164  ?  Ss  11:30  0:00 ./test_daemon
 *                                         ^
 *                                         |
 *    El "?" significa que NO tiene terminal asociada (es daemon!)
 * 
 * 4. Comprueba el PPID (Parent Process ID):
 *    ps -p <PID> -o pid,ppid,cmd
 * 
 *    El PPID debe ser 1 (init/systemd)
 *    Esto significa que el proceso es huérfano → es daemon!
 * 
 * 5. Ver el log:
 *    cat /tmp/test_daemon.log
 *    tail -f /tmp/test_daemon.log  (para ver en tiempo real)
 * 
 * 6. Terminar el daemon:
 *    kill -15 $(cat /tmp/test_daemon.pid)
 *    
 *    O manualmente:
 *    kill -15 <PID>
 * 
 * CARACTERÍSTICAS DE UN DAEMON:
 * =============================
 * ✓ No tiene terminal asociada (TTY = ?)
 * ✓ PPID = 1 (padre es init/systemd)
 * ✓ Corre en background
 * ✓ No responde a Ctrl+C
 * ✓ Stdin, Stdout, Stderr cerrados
 * ✓ Es líder de su propia sesión (setsid)
 */