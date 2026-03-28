#pragma once
#include "Connection.hpp"
#include <csignal>
#include <string>

class BenAfk
{
    public:

        struct Config 
        {
            std::string host = "127.0.0.1";
            int port = 4242;
        };

        BenAfk(const Config& cfg);
        ~BenAfk();

        BenAfk() = delete;
        BenAfk(const BenAfk&) = delete;
        BenAfk(BenAfk&&) = delete;
        BenAfk& operator=(const BenAfk&) = delete;
        BenAfk& operator=(BenAfk&&) = delete;

        void init();
        void run();

    private:

        Connection m_connection;
        int m_signal_fd;
        bool m_running;

        void setupSignals();
        void mainLoop();
        void handleSignal();
        void handleUserInput();
        [[nodiscard]] bool checkServerConnection(int socket_fd);
        void setupFdSet(fd_set& read_fds, int socket_fd);
        [[nodiscard]] int getMaxFd(int socket_fd) const;
};