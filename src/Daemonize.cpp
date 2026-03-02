#include "Daemonize.hpp"
#include "TintinReporter.hpp"
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdlib>
#include <stdexcept>

// Anonymous namespace - Private helpers

namespace
{
    void performFork()
    {
        pid_t pid = fork();
        if(pid < 0)
            throw std::runtime_error("fork() failed");

        if(pid > 0)
            _exit(0);
    }

    void newSession()
    {
        if(setsid() < 0)
            throw std::runtime_error("setsid() failed");
    }

    void changeWorkingDir()
    {
        if(chdir("/") < 0)
            throw std::runtime_error("chdir(\"/\") failed");
    }

    void redirectFd()
    {
        int dev_null = open("/dev/null", O_RDWR);
        if (dev_null < 0)
            throw std::runtime_error("Cannot open /dev/null");

        if (dup2(dev_null, STDIN_FILENO) < 0 ||
            dup2(dev_null, STDOUT_FILENO) < 0 ||
            dup2(dev_null, STDERR_FILENO) < 0)
        {
            close(dev_null);
            throw std::runtime_error("Failed to redirect standard FDs");
        }

        if (dev_null > STDERR_FILENO)
            close(dev_null);
    }
}

void Daemonize::daemonize(TintinReporter& logger)
{
    performFork();
    newSession();
    performFork();
    changeWorkingDir();
    umask(0);
    redirectFd();
    logger.log(TintinReporter::LogLevel::Info, "Standard FDs redirected to /dev/null");       
    logger.log(TintinReporter::LogLevel::Info, "Entering Daemon mode.");
}

void Daemonize::requireRoot()
{
    if (getuid() != 0)
    {
        throw std::runtime_error("Matt_daemon must be run as root (current UID: " + std::to_string(getuid()) + ", expected: 0)");
    }
}