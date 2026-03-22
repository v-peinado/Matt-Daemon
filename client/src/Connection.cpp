#include "Connection.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <stdexcept>
#include <netinet/in.h>  // sockaddr_in
#include <arpa/inet.h>   // inet_pton
#include <cstring>       // memset

// Constructors

Connection::Connection(const std::string& host, int port)
    : m_host(host)
    , m_port(port)
    , m_socket(-1)
    , m_connected(false)
{}

Connection::~Connection()
{
    if(m_socket > -1)
        close(m_socket);
}


// Private methods

void Connection::createSocket()
{
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket < 0)
        throw std::runtime_error("socket() fail");
}

// Public methods

void Connection::connectTo()
{
    createSocket();
    
    struct sockaddr_in server_addr;  // Destiny 127.0.0.0::4242
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(m_port);

    if (inet_pton(AF_INET, m_host.c_str(), &server_addr.sin_addr) <= 0)
        throw std::runtime_error("inet_pton() failed");

    if (connect(m_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
        throw std::runtime_error("connect() fail");
    m_connected = true;
} 

