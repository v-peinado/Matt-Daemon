#pragma once
#include <vector>
#include <sys/select.h>

class Logger;

class Server
{
    public:

        Server() = delete;
        Server(int port, Logger& logger);
        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        Server(Server&&) = delete;
        Server& operator=(Server&&) = delete;
        ~Server() = default;

        bool init();
        void run();
        void stop();
    
    private:

        int m_port;
        int m_server_fd;
        Logger& m_logger;
        bool m_runnig;
        std::vector<int> m_client_fds;
        fd_set m_readFds;

        // Server
};