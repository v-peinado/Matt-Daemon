#pragma once
#include <vector>
#include <string>
#include <sys/select.h>

class TintinReporter;

class Server
{
    public:

        Server() = delete;
        Server(TintinReporter& logger);
        Server(int port, TintinReporter& logger);
        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        Server(Server&&) = delete;
        Server& operator=(Server&&) = delete;
        ~Server();

        bool init();
        void run();
        void stop();
        bool isRunning() const;
    
    private:

        int                 m_port;
        int                 m_server_fd;
        int                 m_signal_fd;
        TintinReporter&     m_logger;
        bool                m_running;
        std::vector<int>    m_client_fds;
        fd_set              m_read_fds;

        // Server setup
        bool createSocket();
        bool bindSocket();
        bool listenSocket();
        bool setupSignals();

        // Signal handler
        void handleSignal();
        std::string getSignalName(int signum);

        // Client management
        void acceptNewClient();
        void handleClientData(int clientFd);
        void disconnectClient(int clientFd);
        bool canAcceptClient() const;

        // Message processing
        void processMessage(int clientFd, const std::string& msg);

        // Helpers
        int getMaxFd() const;
        void setupFdSet();
};