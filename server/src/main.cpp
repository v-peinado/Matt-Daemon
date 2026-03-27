#include "MattDaemon.hpp"
#include "Daemonize.hpp"
#include <iostream>
#include <exception>
#include <memory>

int main()
{
    std::unique_ptr<TintinReporter> logger;
    try
    {
        Daemonize::requireRoot();
        
        // Configuraciones de Matt_daemon
        TintinReporter::Config logger_config {
            .log_file = "/var/log/matt_daemon/matt_daemon.log",
            .application_name = "Matt_daemon",
            .max_size = 10 * 1024 * 1024,
            .max_age_days = 30,
        };
        
        Server::Config server_config {
            .port = 4242,
            .max_clients = 3,
            .buffer_size = 1024
        };
        
        MattDaemon::Config daemon_config {
            .lock_file = "/var/lock/matt_daemon.lock"
        };
        
        logger = std::make_unique<TintinReporter>(logger_config);
        MattDaemon daemon(daemon_config, server_config, *logger);
        
        daemon.init();
        daemon.run();
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << '\n';
        if (logger && logger->isOpen())
        {
            logger->log(TintinReporter::LogLevel::Error, e.what());
            logger->log(TintinReporter::LogLevel::Info, "Quitting.");
        }
        return 1;
    }
}