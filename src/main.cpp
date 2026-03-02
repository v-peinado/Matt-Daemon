#include "MattDaemon.hpp"
#include "Config.hpp"
#include <iostream>
#include <exception>
#include <memory>
#include "Daemonize.hpp" 

int main()
{
    std::unique_ptr<TintinReporter> logger;
    try
    {
        Daemonize::requireRoot();
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
            logger->log(TintinReporter::LogLevel::Error, e.what());
            logger->log(TintinReporter::LogLevel::Info, "Quitting.");
        }
        return 1;
    }
}