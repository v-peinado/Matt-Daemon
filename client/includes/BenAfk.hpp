#pragma once
#include "ArgParser.hpp"
#include "Connection.hpp"
#include <csignal>
#include <string>

class BenAfk
{
    public:

        BenAfk(const ArgParser::parseStruct& arguments);
        ~BenAfk() = default;

        BenAfk() = delete;
        BenAfk(const BenAfk&) = delete;
        BenAfk(BenAfk&&) = delete;
        BenAfk& operator=(const BenAfk&) = delete;
        BenAfk& operator=(BenAfk&&) = delete;

        void init();
        void run();

    private:

        Connection m_connection;

        void setupSignals();
        static volatile sig_atomic_t s_running;
        bool isRunning() const;
        void mainLoop();
};