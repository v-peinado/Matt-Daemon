#include "BenAfk.hpp"
#include <iostream>
#include <csignal>
#include <string>
#include <sys/select.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>

BenAfk::BenAfk(const BenAfk::Config& cfg)
    : m_connection(cfg.host, cfg.port)
    , m_signal_fd(-1)
    , m_running(false) {    
}

BenAfk::~BenAfk() {
    if (m_signal_fd >= 0)
        close(m_signal_fd);
}

void BenAfk::init() {
    m_connection.connectTo();
    setupSignals();
}

void BenAfk::run() {
    mainLoop();
}

void BenAfk::setupSignals() {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    
    if (sigprocmask(SIG_BLOCK, &mask, nullptr) < 0)
        throw std::runtime_error("sigprocmask() failed");
    
    m_signal_fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (m_signal_fd < 0)
        throw std::runtime_error("signalfd() failed");
}

void BenAfk::mainLoop() {
    m_running = true;
    int socket_fd = m_connection.getSocketFd();
    
    while (m_running) {
        fd_set read_fds;
        setupFdSet(read_fds, socket_fd);
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int activity = select(getMaxFd(socket_fd) + 1, &read_fds, nullptr, nullptr, &timeout);
        
        if (activity < 0) {
            if (errno == EINTR)
                continue;
            if (m_running)
                std::cerr << "select() error" << std::endl;
            break;
        }
        
        if (activity == 0)
            continue;
        
        if (FD_ISSET(m_signal_fd, &read_fds)) {
            handleSignal();
            continue;
        }
        
        if (FD_ISSET(socket_fd, &read_fds)) {
            if (!checkServerConnection(socket_fd))
                break;
        }
        
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            handleUserInput();
        }
    }
    
    m_connection.disconnect();
}

void BenAfk::handleSignal() {
    struct signalfd_siginfo siginfo;
    if (read(m_signal_fd, &siginfo, sizeof(siginfo)) == sizeof(siginfo))
        m_running = false;
}

void BenAfk::handleUserInput() {
    std::string line;
    
    if (!std::getline(std::cin, line)) {
        m_running = false;
        return;
    }
    
    if (line.empty())
        return;
    
    try {
        m_connection.sendMsg(line);
    }
    catch (const std::exception&) {
        m_running = false;
    }
}

bool BenAfk::checkServerConnection(int socket_fd)
{
    char buf[1];
    if (recv(socket_fd, buf, sizeof(buf), MSG_PEEK) == 0)
    {
        std::cout << "\nServer closed connection" << std::endl;
        return false;
    }
    return true;
}

void BenAfk::setupFdSet(fd_set& read_fds, int socket_fd)
{
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(socket_fd, &read_fds);
    FD_SET(m_signal_fd, &read_fds);
}

int BenAfk::getMaxFd(int socket_fd) const
{
    int max_fd = socket_fd;
    if (m_signal_fd > max_fd)
        max_fd = m_signal_fd;
    if (STDIN_FILENO > max_fd)
        max_fd = STDIN_FILENO;
    return max_fd;
}