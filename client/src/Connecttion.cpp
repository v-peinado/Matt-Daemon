#include "Connection.hpp"
#include <unistd.h>

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
