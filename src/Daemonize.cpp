#include "Daemonize.hpp"
#include "TintinReporter.hpp"
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdlib>

/* ============================================================================
|                    Anonymous namespace - Private helpers                    |
============================================================================ */

namespace
{
    bool performFork()
    {
        pid_t pid = fork();
        if(pid < 0)
            return false;
        if(pid > 0)
            _exit(0);
        return false;
    }

    bool newSession()
    {
        if(setsid() < 0)
            return false;
        return true;
    }

    bool changeWorkingDir()
    {
        if(chdir("/") < 0)
            return false;
        return true;
    }

    void closeFd()   // no es realmente necesario dup2 ya cierra, es redundancia por seguridad y limpieza
    {
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    } 
}

bool Daemonize::daemonize(TintinReporter& logger)
{
    if(!performFork())
    {
        logger.log(TintinReporter::LogLevel::Error, "First fork() failed");
        return false;
    }
    logger.log(TintinReporter::LogLevel::Info, "First fork() successful");   
    if(!newSession())
    {
        logger.log(TintinReporter::LogLevel::Error, "setsid() failed");
        return false;
    }
    logger.log(TintinReporter::LogLevel::Info, "New session created");   
    if(!performFork())
    {
        logger.log(TintinReporter::LogLevel::Error, "Second fork() failed");
        return false;
    }
    logger.log(TintinReporter::LogLevel::Info, "Second fork() successful");   
    if(!changeWorkingDir())
    {
        logger.log(TintinReporter::LogLevel::Error, "chdir(\"/\") failed");
        return false;
    }
    logger.log(TintinReporter::LogLevel::Info, "Changed directory to /");  
    umask(0);
    logger.log(TintinReporter::LogLevel::Info, "File creation mask reset");
    closeFd();
    logger.log(TintinReporter::LogLevel::Info, "File descriptors closed");
    if(!redirectFd(logger))
    {
        logger.log(TintinReporter::LogLevel::Error, "Failed to redirect standard FDs");
        return false;
    }
    logger.log(TintinReporter::LogLevel::Info, "Standard FDs redirected to /dev/null");       
    logger.log(TintinReporter::LogLevel::Info, "Entering Daemon mode");
    return true;
}