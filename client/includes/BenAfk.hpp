#pragma once
#include "Connection.hpp"
#include <csignal>
#include <string>

class BenAfk
{
    public:

        struct Config 
        {
            std::string     host = "127.0.0.1";
            int             port = 4242;
        };

        BenAfk(const Config& cfg);
        ~BenAfk() = default;

        BenAfk() = delete;
        BenAfk(const BenAfk&) = delete;
        BenAfk(BenAfk&&) = delete;
        BenAfk& operator=(const BenAfk&) = delete;
        BenAfk& operator=(BenAfk&&) = delete;

        static volatile sig_atomic_t s_running;

        void init();
        void run();

    private:

        Connection m_connection;

        void setupSignals();
        [[nodiscard]] bool isRunning() const;
        void mainLoop();
};