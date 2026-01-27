#pragma once
#include <vector>
#include <sys/select.h>

class TintinReporter;

class Server
{
    public:

        Server() = delete;
        Server(int port, TintinReporter& logger);
        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        Server(Server&&) = delete;
        Server& operator=(Server&&) = delete;
        ~Server();

        bool init();
        void run();
        void stop();
    
    private:

        int m_port;
        int m_server_fd;
        TintinReporter& m_logger;
        bool m_runnig;
        std::vector<int> m_client_fds;
        fd_set m_readFds;

        // Server
        bool createSocket();
        bool bindSocket();
        bool listenSocket();

        // Client
        void acceptNewClient();
        void handleClient(int fd);
        void diconnetctClient(int fd);
        bool isMaxClient() const;

        // Message
        void processMsg(int clientfd, const std::string& msg);

        // Helpers
        int getMaxFd() const;
        void setupFdSet();
};