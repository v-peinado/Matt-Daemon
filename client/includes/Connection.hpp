#pragma once



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

    private:

        std::string     m_host;
        int             m_port;
        int             m_socket;
        bool            m_connected;

};