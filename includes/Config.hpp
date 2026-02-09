#pragma once

#include <string_view>

namespace Config
{
    // Server
    constexpr int                   SERVER_PORT = 4242;
    constexpr int                   MAX_CLIENTS = 3;
    constexpr int                   BUFFER_SIZE = 1024;
    constexpr std::string_view      DAEMON_NAME = "Matt_daemon";

    // Paths
    constexpr std::string_view      LOCK_FILE = "/var/lock/matt_daemon.lock";
    constexpr std::string_view      LOG_DIR = "/var/log/matt_daemon";
    constexpr std::string_view      LOG_FILE = "/var/log/matt_daemon/matt_daemon.log";
}

