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

// MattDaemon - Constructor/Destructor

MattDaemon::MattDaemon(TintinReporter& logger)
    : MattDaemon(logger, Config::SERVER_PORT)
{
}

MattDaemon::MattDaemon(TintinReporter& logger, int port)
    : m_logger(logger)
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
        m_logger.log(TintinReporter::LogLevel::Info, "Quitting.");
    }
}

// MattDaemon - Initialization

void MattDaemon::init()
{
    m_logger.log(TintinReporter::LogLevel::Info, "Started.");
    checkRoot();            // Check root privileges
    createLockFile();

    m_server.init();

    m_initialized = true;
}

void MattDaemon::run()
{
    if (!m_initialized)
        throw std::runtime_error("Cannot run: not initialized");
    
    Daemonize::daemonize(m_logger);
  
    m_logger.log(TintinReporter::LogLevel::Info, "started. PID: " + std::to_string(getpid()) + ".");
    
    m_server.run();
}


void MattDaemon::checkRoot()  
{
    if (getuid() != 0)
    {
        m_logger.log(TintinReporter::LogLevel::Error, "Must be run as root (current UID: " + std::to_string(getuid()) + ")");
        throw std::runtime_error("Must be run as root");
    }
    
    m_logger.log(TintinReporter::LogLevel::Info, "Root privileges confirmed");
}

void MattDaemon::createLockFile()  
{
    // Try to open/create lock file
    m_lock_fd = open(std::string(Config::LOCK_FILE).c_str(), O_CREAT | O_RDWR, 0644);
    
    if (m_lock_fd < 0)
    {
        m_logger.log(TintinReporter::LogLevel::Error, "Cannot open lock file: " + std::string(strerror(errno)));
        throw std::runtime_error("Cannot open lock file");
    }
    
    // Try to acquire exclusive lock
    if (flock(m_lock_fd, LOCK_EX | LOCK_NB) < 0)
    {
        // Lock failed - another instance is running
        if (errno == EWOULDBLOCK)
        {
            std::cerr << "Error: Another instance is already running" << std::endl;
            std::cerr << "Can't open :" << Config::LOCK_FILE << std::endl;

            m_logger.log(TintinReporter::LogLevel::Error, "Error file locked.");
            
            close(m_lock_fd);
            m_lock_fd = -1;
            throw std::runtime_error("Lock file already locked (another instance running)");
        }
        
        std::cerr << "Error: flock() failed: " << strerror(errno) << std::endl;
        
        m_logger.log(TintinReporter::LogLevel::Error, "flock() failed: " + std::string(strerror(errno)));
        
        close(m_lock_fd);
        m_lock_fd = -1;
        throw std::runtime_error("flock() failed");
    }

    m_logger.log(TintinReporter::LogLevel::Info, "Lock file created: " + std::string(Config::LOCK_FILE));
}

void MattDaemon::removeLockFile()
{
    if (m_lock_fd >= 0)
    {
        flock(m_lock_fd, LOCK_UN);
        close(m_lock_fd);
        m_lock_fd = -1;
        unlink(std::string(Config::LOCK_FILE).c_str());
        m_logger.log(TintinReporter::LogLevel::Info, "Lock file removed");
    }
}