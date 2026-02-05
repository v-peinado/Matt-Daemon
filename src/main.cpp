#include "MattDaemon.hpp"
#include "Config.hpp"
#include <iostream>
#include <exception>

int main()
{
    try
    {
        MattDaemon daemon;
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