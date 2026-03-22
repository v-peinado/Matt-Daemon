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

        static volatile sig_atomic_t s_running;

        void init();
        void run();

    private:

        Connection m_connection;

        void setupSignals();
        bool isRunning() const;
        void mainLoop();
};