#include "BenAfk.hpp"
#include <iostream>
#include <csignal>
#include <string>

// Static members

volatile sig_atomic_t BenAfk::s_running = 1;

static void signalHandler(int signum)
{
    (void)signum;
    BenAfk::s_running = 0;
}

// Constructors

BenAfk::BenAfk(const ArgParser::parseStruct& arguments)
    : m_connection(arguments.host, arguments.port)
{    
}

// Public methods

void BenAfk::init()
{
    m_connection.connectTo();
}

void BenAfk::run()
{
    setupSignals();
    mainLoop();
}

// Private methods

void BenAfk::setupSignals()
{
    signal(SIGINT, signalHandler);
}

bool BenAfk::isRunning() const
{
    return s_running;
}

void BenAfk::mainLoop()
{
    while(isRunning())
    {
        std::string line;
        std::getline(std::cin, line);

        if (std::cin.eof()) // Ctrl+D
        {  
            s_running = 0;
            break;
        }

        if (line.empty())
            continue;

        if(line == "quit")
            s_running = 0;

        m_connection.sendMsg(line);
    }
    m_connection.disconnect();
}
