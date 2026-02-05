#pragma once

class TintinReporter;

namespace Daemonize
{
    void daemonize(TintinReporter& logger);                     // Convert current process into a daemon, main function
}