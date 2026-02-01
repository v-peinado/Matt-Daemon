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

void MattDaemon::init()
{
    m_logger.log(TintinReporter::LogLevel::Info, "Started");
    checkRoot();            // Check root privileges
    createLockFile();
    m_logger.log(TintinReporter::LogLevel::Info, "Creating server.");
    
    if (!m_server.init())
    {
        m_logger.log(TintinReporter::LogLevel::Error, "Server initialization failed");
        throw std::runtime_error("Server initialization failed");
    }
    m_logger.log(TintinReporter::LogLevel::Info, "Server created.");
    m_initialized = true;
}

void MattDaemon::run()
{
    if (!m_initialized)
    {
        m_logger.log(TintinReporter::LogLevel::Error, "Cannot run: not initialized");
        throw std::runtime_error("Cannot run: not initialized");
    }
    
    // Daemonize puede lanzar exceptions
    try
    {
        Daemonize::daemonize(m_logger);
    }
    catch (const std::runtime_error& e)
    {
        // AQUÍ logueamos
        m_logger.log(TintinReporter::LogLevel::Error, 
                     "Daemonization failed: " + std::string(e.what()));
        throw;  // Re-lanzar
    }
    
    pid_t pid = getpid();
    m_logger.log(TintinReporter::LogLevel::Info, 
                 "started. PID: " + std::to_string(pid));
    
    m_server.run();
    
    m_logger.log(TintinReporter::LogLevel::Info, "Quitting.");
}


void MattDaemon::checkRoot()  // ← void, throws
{
    if (getuid() != 0)
    {
        std::cerr << "Error: Matt_daemon must be run as root" << std::endl;
        
        // LOG + THROW
        m_logger.log(TintinReporter::LogLevel::Error, 
                     "Must be run as root (current UID: " + std::to_string(getuid()) + ")");
        throw std::runtime_error("Must be run as root");
    }
    
    m_logger.log(TintinReporter::LogLevel::Info, "Root privileges confirmed");
}

void MattDaemon::createLockFile()  // ← void, throws
{
    // Try to open/create lock file
    m_lock_fd = open(Config::LOCK_FILE.c_str(), O_CREAT | O_RDWR, 0644);
    
    if (m_lock_fd < 0)
    {
        std::cerr << "Error: Cannot open lock file: " << Config::LOCK_FILE << std::endl;
        
        // LOG + THROW
        m_logger.log(TintinReporter::LogLevel::Error, 
                     "Cannot open lock file: " + std::string(strerror(errno)));
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
            
            // LOG + THROW
            m_logger.log(TintinReporter::LogLevel::Error, "Error file locked.");
            
            close(m_lock_fd);
            m_lock_fd = -1;
            throw std::runtime_error("Lock file already locked (another instance running)");
        }
        
        // Other error
        std::cerr << "Error: flock() failed: " << strerror(errno) << std::endl;
        
        // LOG + THROW
        m_logger.log(TintinReporter::LogLevel::Error, 
                     "flock() failed: " + std::string(strerror(errno)));
        
        close(m_lock_fd);
        m_lock_fd = -1;
        throw std::runtime_error("flock() failed");
    }
    
    m_logger.log(TintinReporter::LogLevel::Info, "Lock file created: " + Config::LOCK_FILE);
}

void MattDaemon::removeLockFile()
{
    if (m_lock_fd >= 0)
    {
        flock(m_lock_fd, LOCK_UN);
        close(m_lock_fd);
        m_lock_fd = -1;
        unlink(Config::LOCK_FILE.c_str());
        m_logger.log(TintinReporter::LogLevel::Info, "Lock file removed");
    }
}