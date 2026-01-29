#include "Server.hpp"
#include "Config.hpp"
#include "TintinReporter.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <cerrno>
#include <sys/signalfd.h>
#include <signal.h>

// Constructor/Destructor

Server::Server(int port, TintinReporter& logger)
    : m_port(port)
    , m_server_fd(-1)
    , m_signal_fd(-1)
    , m_logger(logger)
    , m_running(false)
{
    m_logger.log(TintinReporter::LogLevel::Info, "Server object created");
}

Server::~Server()
{
    stop();
    if (m_signal_fd >= 0)
    {
        close(m_signal_fd);
        m_logger.log(TintinReporter::LogLevel::Info, "Signal fd closed");
    }
    if (m_server_fd >= 0)
    {
        close(m_server_fd);
        m_logger.log(TintinReporter::LogLevel::Info, "Server closed");
    }
}

// Server initialization 

bool Server::init()
{
    m_logger.log(TintinReporter::LogLevel::Info, "Creating server...");
    if (!createSocket())
        return false;
    if (!bindSocket())
        return false;
    if (!listenSocket())
        return false;
    if (!setupSignals())
        return false;
    m_logger.log(TintinReporter::LogLevel::Info, "Server created.");
    return true;
}

bool Server::createSocket()
{
    m_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (m_server_fd < 0)
    {
        m_logger.log(TintinReporter::LogLevel::Error, "socket() failed: " + std::string(strerror(errno)));
        return false;
    }
    
    int opt = 1;
    if (setsockopt(m_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        m_logger.log(TintinReporter::LogLevel::Warning, "setsockopt(SO_REUSEADDR) failed");
    
    m_logger.log(TintinReporter::LogLevel::Info, "Socket created");
    return true;
}

bool Server::bindSocket()
{
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(m_port);

    if (bind(m_server_fd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        m_logger.log(TintinReporter::LogLevel::Error, "bind() failed: " + std::string(strerror(errno)));
        close(m_server_fd);
        m_server_fd = -1;
        return false;
    }
    
    m_logger.log(TintinReporter::LogLevel::Info, "Socket bound to port " + std::to_string(m_port));
    return true;
}

bool Server::listenSocket()
{
    if (listen(m_server_fd, 5) < 0)
    {
        m_logger.log(TintinReporter::LogLevel::Error, "listen() failed: " + std::string(strerror(errno)));
        close(m_server_fd);
        m_server_fd = -1;
        return false;
    }
    
    m_logger.log(TintinReporter::LogLevel::Info, "Server listening on port " + std::to_string(m_port));
    return true;
}

bool Server::setupSignals()
{
    sigset_t mask;
    sigemptyset(&mask);             // Init signal set

    // Add signals to handle
    sigaddset(&mask, SIGTERM);      // Terminatiopn signal
    sigaddset(&mask, SIGINT);       // Interrupt Ctrl + C
    sigaddset(&mask, SIGQUIT);      // Quit
    sigaddset(&mask, SIGHUP);       // Hangup

    // Block signals, only delivered by signalfd
    if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0)
    {
        m_logger.log(TintinReporter::LogLevel::Error, "sigprocmask() failed: " + std::string(strerror(errno)));
        return false;
    }
    // Create signal fd
    m_signal_fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (m_signal_fd < 0)
    {
        m_logger.log(TintinReporter::LogLevel::Error, "signalfd() failed: " + std::string(strerror(errno)));
        return false;
    }  
    m_logger.log(TintinReporter::LogLevel::Info, "Signal handling configured (signalfd)");
    return true;
}

// Signal handler

std::string Server::getSignalName(int signum)
{
    switch (signum)
    {
        case SIGTERM:   return "SIGTERM";
        case SIGINT:    return "SIGINT";
        case SIGQUIT:   return "SIGQUIT";
        case SIGHUP:    return "SIGHUP";
        default:        return "UNKNOWN(" + std::to_string(signum) + ")";
    }
}

void Server::handleSignal()
{
    struct signalfd_siginfo siginfo; 
    ssize_t bytes = read(m_signal_fd, &siginfo, sizeof(siginfo));
    if (bytes != sizeof(siginfo))
    {
        m_logger.log(TintinReporter::LogLevel::Error, "Failed to read signal info");
        return;
    }
    
    int signum = siginfo.ssi_signo;
    std::string signame = getSignalName(signum);   
    m_logger.log(TintinReporter::LogLevel::Info, "Received signal: " + signame);
    
    // Stop server on any of these signals
    m_logger.log(TintinReporter::LogLevel::Info, "Shutting down...");
    stop();
}

// Server main loop

void Server::run()
{
    m_running = true;
    m_logger.log(TintinReporter::LogLevel::Info, "Server started");
    
    while (m_running)
    {
        setupFdSet();
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0; 
        int activity = select(getMaxFd() + 1, &m_read_fds, NULL, NULL, &timeout);

        if (activity < 0)
        {
            if (errno == EINTR)
                continue;
            
            m_logger.log(TintinReporter::LogLevel::Error, "select() failed: " + std::string(strerror(errno)));
            break;
        }

        if (activity == 0)
            continue;

        if (FD_ISSET(m_signal_fd, &m_read_fds))
        {
            handleSignal();
            continue;
        }

        if (FD_ISSET(m_server_fd, &m_read_fds))
            acceptNewClient();

        for (size_t i = 0; i < m_client_fds.size(); )
        {
            int clientFd = m_client_fds[i];
            if (FD_ISSET(clientFd, &m_read_fds))
                handleClientData(clientFd);
            if (i < m_client_fds.size() && m_client_fds[i] == clientFd)
                i++;
        }
    }
    m_logger.log(TintinReporter::LogLevel::Info, "Server stopped");
}

void Server::stop()
{
    if (!m_running)
        return;
    m_running = false;

    for (size_t i = 0; i < m_client_fds.size(); i++)
        close(m_client_fds[i]);

    m_client_fds.clear();
    m_logger.log(TintinReporter::LogLevel::Info, "All clients disconnected");
}

bool Server::isRunning() const
{
    return m_running;
}

// Client management

void Server::acceptNewClient()
{
    if (!canAcceptClient())
    {
        m_logger.log(TintinReporter::LogLevel::Warning, "Maximum clients reached, rejecting connection");
        int clientFd = accept(m_server_fd, NULL, NULL);
        if (clientFd >= 0)
            close(clientFd);
        return;
    }

    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    int clientFd = accept(m_server_fd, (struct sockaddr*)&clientAddr, &addrLen);

    if (clientFd < 0)
    {
        m_logger.log(TintinReporter::LogLevel::Error, "accept() failed: " + std::string(strerror(errno)));
        return;
    }
    m_client_fds.push_back(clientFd);
    m_logger.log(TintinReporter::LogLevel::Info, "Client connected from " + std::string(inet_ntoa(clientAddr.sin_addr)) + ":" + std::to_string(ntohs(clientAddr.sin_port)));
}

void Server::handleClientData(int clientFd)
{
    char buffer[Config::BUFFER_SIZE];
    std::memset(buffer, 0, sizeof(buffer));
    ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesRead <= 0)
    {
        if (bytesRead == 0)
            m_logger.log(TintinReporter::LogLevel::Info, "Client disconnected");
        else
            m_logger.log(TintinReporter::LogLevel::Error, "recv() failed: " + std::string(strerror(errno))); 
        disconnectClient(clientFd);
        return;
    }

    std::string message(buffer, bytesRead);

    if (!message.empty() && message[message.length() - 1] == '\n')
        message.erase(message.length() - 1); 
    processMessage(clientFd, message);
}

void Server::disconnectClient(int clientFd)
{
    for (size_t i = 0; i < m_client_fds.size(); i++)
    {
        if (m_client_fds[i] == clientFd)
        {
            close(clientFd);
            m_client_fds.erase(m_client_fds.begin() + i);
            m_logger.log(TintinReporter::LogLevel::Info, "Client removed from list");
            return;
        }
    }
}

bool Server::canAcceptClient() const
{
    return m_client_fds.size() < static_cast<size_t>(Config::MAX_CLIENTS);
}

// Message processing

void Server::processMessage(int clientFd, const std::string& message)
{
    (void)clientFd;

    if (message == "quit")
    {
        m_logger.log(TintinReporter::LogLevel::Info, "Request quit.");
        stop();
    }
    else
        m_logger.log(TintinReporter::LogLevel::Log, "User input: " + message);   
}

// Helper functions 

int Server::getMaxFd() const
{
    int maxFd = m_server_fd;

    if (m_signal_fd > maxFd)
        maxFd = m_signal_fd;   
    for (size_t i = 0; i < m_client_fds.size(); i++)
    {
        if (m_client_fds[i] > maxFd)
            maxFd = m_client_fds[i];
    }
    return maxFd;
}

void Server::setupFdSet()
{
    FD_ZERO(&m_read_fds);
    FD_SET(m_server_fd, &m_read_fds);

    if (m_signal_fd >= 0)
        FD_SET(m_signal_fd, &m_read_fds);
    for (size_t i = 0; i < m_client_fds.size(); i++)
        FD_SET(m_client_fds[i], &m_read_fds);
}