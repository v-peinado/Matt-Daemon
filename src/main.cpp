#include "MattDaemon.hpp"
#include "Config.hpp"
#include <iostream>
#include <exception>
#include <memory>

int main()
{
    std::unique_ptr<TintinReporter> logger;
    try
    {
        logger = std::make_unique<TintinReporter>();
        MattDaemon daemon(*logger);
        daemon.init();
        daemon.run();
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << '\n';
        if (logger && logger->isOpen())
        {
            try
            {
                logger->log(TintinReporter::LogLevel::Error, e.what());
                logger->log(TintinReporter::LogLevel::Info, "Quitting.");
            }
            catch (...)
            {
                // Si falla el logging, ignorar silenciosamente
                // Ya mostramos el error en cerr
            }
        }
        return 1;
    }
}