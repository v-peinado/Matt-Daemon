#include "MattDaemon.hpp"
#include "Config.hpp"
#include <iostream>
#include <exception>
#include <memory>

int main()
{
    try
    {
        TintinReporter logger;
        MattDaemon daemon(logger);
        daemon.init();
        daemon.run();
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
}