#pragma once
#include "TintinReporter.hpp"
#include "Server.hpp"

class MattDaemon
{
    public:

        struct Config
        {
            std::string lock_file = "/var/lock/daemon.lock";
        };

        MattDaemon(const Config& cfg, const Server::Config& srv_cfg, TintinReporter& logger);

        MattDaemon() = delete;
        MattDaemon(const MattDaemon&) = delete;
        MattDaemon& operator=(const MattDaemon&) = delete;
        MattDaemon(MattDaemon&&) = delete;
        MattDaemon& operator=(MattDaemon&&) = delete;
        ~MattDaemon();
        
        void init();
        void run();

    private:

        std::string         m_lock_file;
        TintinReporter&     m_logger;
        Server              m_server;
        int                 m_lock_fd;
        bool                m_initialized;

        // Initialization helpers
        void createLockFile();
        void removeLockFile();
};
