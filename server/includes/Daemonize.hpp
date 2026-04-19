#pragma once

class TintinReporter;

namespace Daemonize {
    void requireRoot(); 
    void daemonize(TintinReporter& logger);                     // Convert current process into a daemon, main function
}