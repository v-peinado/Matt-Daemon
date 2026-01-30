#include "MattDaemon.hpp"
#include "Daemonize.hpp"
#include "Config.hpp"
#include <unistd.h>
#include <sys/file.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>

// Constructor/Destructor

MattDaemon::MattDaemon()
    : MattDaemon(Config::SERVER_PORT)
{
}

MattDaemon::MattDaemon(int port)
    : m_logger()
    , m_server(port, m_logger)
    , m_lock_fd(-1)
    , m_initialized(false)
{
}

MattDaemon::~MattDaemon()
{
    removeLockFile();
    
    if (m_initialized)
    {
        m_logger.log(TintinReporter::LogLevel::Info, "Daemon stopped");
    }
}

// Initialization

