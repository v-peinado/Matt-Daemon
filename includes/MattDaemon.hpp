#pragma once
#include "TintinReporter.hpp"
#include "Server.hpp"

class MattDaemon
{
    public:

        MattDaemon();
        MattDaemon(int port);
        MattDaemon(const MattDaemon&) = delete;
        MattDaemon& operator=(const MattDaemon&) = delete;
        MattDaemon(MattDaemon&&) = delete;
        MattDaemon& operator=(MattDaemon&&) = delete;
        ~MattDaemon();

        void init();
        void run();

    private:

        TintinReporter  m_logger;
        Server          m_server;
        int             m_lock_fd;
        bool            m_initialized;

        // Initialization helpers
        void checkRoot();
        void createLockFile();
        void removeLockFile();
};
