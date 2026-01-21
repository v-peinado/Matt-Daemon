#pragma once

#include <string>

namespace Config
{
    // Server
    const int           SERVER_PORT = 4242;
    const int           MAX_CLIENTS = 3;
    const int           BUFFER_SIZE = 1024;
    const std::string   DAEMON_NAME = "Matt_daemon";

    // Paths
    const std::string   LOCK_FILE = "/var/lock/matt_daemon.lock";
    const std::string   LOG_DIR = "/var/log/matt_daemon";
    const std::string   LOG_FILE = "/var/log/matt_daemon/matt_daemon.log";
}

