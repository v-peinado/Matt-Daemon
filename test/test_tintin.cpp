#include "TintinReporter.hpp"
#include <iostream>

int main()
{
    std::cout << "=== Test TintinReporter ===" << std::endl;
    
    // Crear el objeto
    std::cout << "Creando TintinReporter..." << std::endl;
    std::string logPath = "test_matt_daemon.log";
    TintinReporter reporter(logPath); // mejor en temp
    
    if (!reporter.isOpen()) {
        std::cerr << "Error: No se pudo abrir el archivo de log" << std::endl;
        return 1;
    }
    
    std::cout << "TintinReporter creado correctamente" << std::endl;
    
    // pruebo cada tipo de log
    std::cout << "\nHaciendo logs de prueba..." << std::endl;
    
    reporter.log(TintinReporter::LogLevel::Info, "Daemon iniciado");
    reporter.log(TintinReporter::LogLevel::Log, "Usuario escribió: Hola mundo");
    reporter.log(TintinReporter::LogLevel::Warning, "Conexión lenta detectada");
    reporter.log(TintinReporter::LogLevel::Error, "Error al procesar mensaje");
    reporter.log(TintinReporter::LogLevel::Info, "Daemon finalizando");
    
    std::cout << "Logs escritos correctamente" << std::endl;
    
    std::cout << "\n=== Test completado ===" << std::endl;
    std::cout << "Ver log en: " <<  logPath << std::endl;
    std::cout << "Comando: cat " << logPath << std::endl;
    
    return 0;
}