#pragma once
#include <string>

class Connection {
    public:

        Connection(const std::string& host, int port);
        ~Connection();

        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;

        void connectTo();
        void disconnect();
        void sendMsg(const std::string& msg);
        [[nodiscard]] int getSocketFd() const;

    private:

        std::string m_host;
        int m_port;
        int m_socket;
        bool m_connected;

        void createSocket();
};