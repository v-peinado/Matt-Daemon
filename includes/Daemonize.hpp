#pragma once

class TintinReporter;

namespace Daemonize
{
    bool daemonize(TintinReporter& logger);                     // Convert current process into a daemon, main function
}