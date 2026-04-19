#include "Server.hpp"
#include <vector>
#include "TintinReporter.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <cerrno>
#include <sys/signalfd.h>
#include <signal.h>

// Server - Constructor/Destructor

Server::Server(const Config& cfg, TintinReporter& logger)
    : m_port(cfg.port)
    , m_max_clients(cfg.max_clients)
    , m_buffer_size(cfg.buffer_size)
    , m_server_fd(-1)
    , m_signal_fd(-1)
    , m_logger(logger)
    , m_running(false) {
    m_logger.log(TintinReporter::LogLevel::Info, "Server object created");
}

Server::~Server() {
    stop();
    if (m_signal_fd >= 0) {
        close(m_signal_fd);
        m_logger.log(TintinReporter::LogLevel::Info, "Signal fd closed");
    }

    if (m_server_fd >= 0) {
        close(m_server_fd);
        m_logger.log(TintinReporter::LogLevel::Info, "Server closed");
    }
}

// Server - Initialization 

void Server::init() {
    m_logger.log(TintinReporter::LogLevel::Info, "Creating server...");
    createSocket();
    bindSocket();
    listenSocket();
    setupSignals();
    m_logger.log(TintinReporter::LogLevel::Info, "Server created.");
}

void Server::createSocket() {
    m_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (m_server_fd < 0)
        throw std::runtime_error("socket() failed: " + std::string(strerror(errno)));
    
    int opt = 1;
    if (setsockopt(m_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        m_logger.log(TintinReporter::LogLevel::Warning, "setsockopt(SO_REUSEADDR) failed");
    
    m_logger.log(TintinReporter::LogLevel::Info, "Socket created");
}

void Server::bindSocket() {
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(m_port);

    if (bind(m_server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(m_server_fd);
        m_server_fd = -1;
        throw std::runtime_error("bind() failed: " + std::string(strerror(errno)));
    }
    
    m_logger.log(TintinReporter::LogLevel::Info, "Socket bound to port " + std::to_string(m_port));
}

void Server::listenSocket() {
    if (listen(m_server_fd, 5) < 0) {
        close(m_server_fd);
        m_server_fd = -1;
        throw std::runtime_error("listen() failed: " + std::string(strerror(errno)));
    }
    
    m_logger.log(TintinReporter::LogLevel::Info, "Server listening on port " + std::to_string(m_port));
}

void Server::setupSignals() {
    sigset_t mask;
    sigemptyset(&mask);             // Init signal set

    // Add signals to handle
    sigaddset(&mask, SIGTERM);      // (15)Terminatiopn signal
    sigaddset(&mask, SIGINT);       // (2)Interrupt Ctrl + C
    sigaddset(&mask, SIGQUIT);      // (3)Quit
    sigaddset(&mask, SIGHUP);       // (1)Hangup

    // SIGKILL(9)  - Kill signal (immediate termination, no cleanup) kill -9
    // SIGSTOP(19) - Stop signal (immediate pause, no cleanup) kill -STOP

    // Block signals, only delivered by signalfd
    if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0)
        throw std::runtime_error("sigprocmask() failed: " + std::string(strerror(errno)));

    // Create signal fd
    m_signal_fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (m_signal_fd < 0)
        throw std::runtime_error("signalfd() failed: " + std::string(strerror(errno)));
    m_logger.log(TintinReporter::LogLevel::Info, "Signal handling configured (signalfd)");
}

// Signal handler

std::string_view Server::getSignalName(int signum) {
    switch (signum) {
        case SIGTERM:   return "SIGTERM";
        case SIGINT:    return "SIGINT";
        case SIGQUIT:   return "SIGQUIT";
        case SIGHUP:    return "SIGHUP";
        default:        return "UNKNOWN";
    }
}

void Server::handleSignal() {
    struct signalfd_siginfo siginfo; 
    ssize_t bytes = read(m_signal_fd, &siginfo, sizeof(siginfo));
    if (bytes != sizeof(siginfo)) {
        m_logger.log(TintinReporter::LogLevel::Error, "Failed to read signal info");
        return;
    }
    
    int signum = siginfo.ssi_signo;
       
    m_logger.log(TintinReporter::LogLevel::Info, "Received signal: " + std::string(getSignalName(signum)));
    
    // Stop server on any of these signals
    m_logger.log(TintinReporter::LogLevel::Info, "Signal handler.");
    stop();
}

// Server main loop

void Server::run() {
    m_running = true;
    m_logger.log(TintinReporter::LogLevel::Info, "Server running");
    
    while (m_running) {
        setupFdSet();
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0; 
        int activity = select(getMaxFd() + 1, &m_read_fds, nullptr, nullptr, &timeout);

        if (activity < 0) {
            if (errno == EINTR) // syscall interrupted by a signal; not an error, retry the call safely
                continue;       // otherwise the server could stop unexpectedly due to harmless signals
            
            throw std::runtime_error("select() failed: " + std::string(strerror(errno)));
            break;
        }

        if (activity == 0)
            continue;

        if (FD_ISSET(m_signal_fd, &m_read_fds)) {
            handleSignal();
            continue;
        }

        if (FD_ISSET(m_server_fd, &m_read_fds))
            acceptNewClient();

        for (size_t i = 0; i < m_client_fds.size(); ) {
            int clientFd = m_client_fds[i];
            if (FD_ISSET(clientFd, &m_read_fds))
                handleClientData(clientFd);
            if (i < m_client_fds.size() && m_client_fds[i] == clientFd)
                i++;
        }
    }
    m_logger.log(TintinReporter::LogLevel::Info, "Server stopped");
}

void Server::stop() {
    if (!m_running)
        return;

    m_running = false;

    for (int fd : m_client_fds)
        close(fd);

    m_client_fds.clear();
    m_logger.log(TintinReporter::LogLevel::Info, "All clients disconnected");
}

bool Server::isRunning() const {
    return m_running;
}

// Client management

void Server::acceptNewClient() {
    if (!canAcceptClient()) {
        m_logger.log(TintinReporter::LogLevel::Warning, "Maximum clients reached, rejecting connection");
        int clientFd = accept(m_server_fd, NULL, NULL);
        if (clientFd >= 0)
            close(clientFd);
        return;
    }

    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    int clientFd = accept(m_server_fd, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);

    if (clientFd < 0)
        throw std::runtime_error("accept() failed: " + std::string(strerror(errno)));
    m_client_fds.push_back(clientFd);
    m_logger.log(TintinReporter::LogLevel::Info, "Client connected from " + std::string(inet_ntoa(clientAddr.sin_addr)) + ":" + std::to_string(ntohs(clientAddr.sin_port)));
}

void Server::handleClientData(int clientFd) {
    std::vector<char> buffer(m_buffer_size);
    ssize_t bytesRead = recv(clientFd, buffer.data(), buffer.size() - 1, 0);
    
    if (bytesRead <= 0) {
        if (bytesRead == 0)
            m_logger.log(TintinReporter::LogLevel::Info, "Client disconnected");
        else
            m_logger.log(TintinReporter::LogLevel::Error, "recv() failed: " + std::string(strerror(errno)));
        disconnectClient(clientFd);
        return;
    }

    std::string message(buffer.data(), bytesRead);

    while (!message.empty() && (message.back() == '\n' || message.back() == '\r'))  // Win and telnet too
        message.pop_back();

    processMessage(clientFd, message);
}

void Server::disconnectClient(int clientFd) {
    for (size_t i = 0; i < m_client_fds.size(); i++) {
        if (m_client_fds[i] == clientFd) {
            close(clientFd);
            m_client_fds.erase(m_client_fds.begin() + i);
            m_logger.log(TintinReporter::LogLevel::Info, "Client removed from list");
            return;
        }
    }
}

bool Server::canAcceptClient() const {
    return m_client_fds.size() < static_cast<size_t>(m_max_clients);
}
// Message processing

void Server::processMessage(int clientFd, const std::string& message) {
    (void)clientFd;

    if (message == "quit") {
        m_logger.log(TintinReporter::LogLevel::Info, "Request quit.");
        stop();
    }
    else
        m_logger.log(TintinReporter::LogLevel::Log, "User input: " + message);  
}

// Helper functions 

int Server::getMaxFd() const {
    int maxFd = std::max(m_server_fd, m_signal_fd);

    for (size_t i = 0; i < m_client_fds.size(); i++) {
        if (m_client_fds[i] > maxFd)
            maxFd = m_client_fds[i];
    }
    return maxFd;
}

void Server::setupFdSet() {
    FD_ZERO(&m_read_fds);
    FD_SET(m_server_fd, &m_read_fds);

    if (m_signal_fd >= 0)
        FD_SET(m_signal_fd, &m_read_fds);
    for (int fd : m_client_fds)
        FD_SET(fd, &m_read_fds);
}