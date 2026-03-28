#include "Connection.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>

Connection::Connection(const std::string& host, int port)
    : m_host(host)
    , m_port(port)
    , m_socket(-1)
    , m_connected(false)
{}

Connection::~Connection()
{
    if (m_socket >= 0)
        close(m_socket);
}

void Connection::createSocket()
{
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket < 0)
        throw std::runtime_error("socket() failed");
}

void Connection::connectTo()
{
    createSocket();
    
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(m_port);

    if (inet_pton(AF_INET, m_host.c_str(), &server_addr.sin_addr) <= 0)
        throw std::runtime_error("inet_pton() failed");

    if (connect(m_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
        throw std::runtime_error("connect() failed");
    
    m_connected = true;
}

void Connection::disconnect()
{
    if (m_socket < 0)
        return;
    
    close(m_socket);
    m_socket = -1;
    m_connected = false;
}

void Connection::sendMsg(const std::string& message)
{
    if (!m_connected)
        throw std::runtime_error("Not connected");
    
    if (send(m_socket, message.c_str(), message.length(), MSG_NOSIGNAL) < 0)
        throw std::runtime_error("send() failed");
}

int Connection::getSocketFd() const
{
    return m_socket;
}