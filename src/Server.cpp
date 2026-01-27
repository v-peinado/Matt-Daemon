#include "Server.hpp"
#include "Config.hpp"
#include "TintinReporter.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <cerrno>

// Constructors

Server::Server(int port, TintinReporter& logger)
    : m_port(port),
    m_logger(logger),
    m_server_fd(-1),
    m_running(false)
{
    m_logger.log(TintinReporter::LogLevel::Info, "Server object created");
}

Server::~Server()
{
    stop();
    if(m_server_fd >= 0)
    {
        close(m_server_fd);
        m_logger.log(TintinReporter::LogLevel::Info, "Server closed");
    }
}

// Server initialization

void Server::init()
{
    m_logger.log(TintinReporter::LogLevel::Info, "Creating server ...");
    if(!createSocket())
        return false;
    if(bindSocket())
        return false;
    if(!listenSocket())
        return false;
    m_logger.log(TintinReporter::LogLevel::Info, "Server created.");
}

bool Server::createSocket()
{
    m_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;

    if (m_server_fd < 0)
    {
        m_logger.log(TintinReporter::LogLevel::Error, "socket() failed: " + std::string(strerror(errno)));
        return false;
    }
    if (setsockopt(m_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
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

    if (bind(m_serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        m_logger.log(TintinReporter::LogLevel::Error, "bind() failed: " + std::string(strerror(errno)));
        close(m_serverFd);
        m_serverFd = -1;
        return false;
    }
    m_logger.log(TintinReporter::LogLevel::Info, "Socket bound to port " + std::to_string(m_port));
    return true;
}

bool Server::listenSocket()
{
    if (listen(m_serverFd, 5) < 0)
    {
        m_logger.log(TintinReporter::LogLevel::Error, "listen() failed: " + std::string(strerror(errno)));
        close(m_serverFd);
        m_serverFd = -1;
        return false;
    }
    m_logger.log(TintinReporter::LogLevel::Info, "Server listening on port " + std::to_string(m_port));
    return true;
}