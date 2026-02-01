#include "MattDaemon.hpp"
#include "Config.hpp"
#include <iostream>
#include <exception>

int main()
{
    try
    {
        // Create daemon
        // TintinReporter constructor may throw
        MattDaemon daemon;
        
        // Initialize components
        // throws std::runtime_error on failure
        daemon.init();
        
        // Run daemon (daemonize + server loop)
        // throws std::runtime_error on failure
        daemon.run();
        
        return 0;
    }
    catch (const std::runtime_error& e)
    {
        // Log-then-throw errors from MattDaemon
        // or TintinReporter constructor failure
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    catch (const std::exception& e)
    {
        // Other unexpected exception
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        // Unknown exception
        std::cerr << "Unknown fatal error" << std::endl;
        return 1;
    }
}