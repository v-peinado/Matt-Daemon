#pragma once
#include <string>
#include <exception>

class Connection
{
    public:

        Connection(const std::string& host, int port);
        Connection() = delete;

        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;
        Connection(Connection&&) = delete;
        Connection& operator=(Connection&&) = delete;
        ~Connection();

        void connectTo();
        void disconnect();
        bool isConnected() const;
        void sendMsg(const std::string& msg);

    private:

        std::string     m_host;
        int             m_port;
        int             m_socket;
        bool            m_connected;

        void createSocket();
};