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
        void readInput(std::string& line);  //quiza se escribva en line
        void proccInput(const std::string& input); // si esta vacio se ignora, se manda el mensaje, y DESPUES, si es quit se avisa y se cierra.
};