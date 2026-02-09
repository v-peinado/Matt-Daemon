#include "TintinReporter.hpp"
#include "Config.hpp"
#include <ctime>
#include <chrono>
#include <unistd.h>
#include <sys/stat.h>
#include <stdexcept>
#include <format>

/* ============================================================================
|                        TintinReporter constructors                          |
============================================================================ */

TintinReporter::TintinReporter() 
    : TintinReporter(std::string(Config::LOG_FILE))
{
    // Body empty, all work in delegated constructor
}

TintinReporter::TintinReporter(const std::string& logfile)
    : m_path_logfile(logfile)
{
    createLogDirectory();

    m_file.open(m_path_logfile, std::ios::out | std::ios::app);
    if(!m_file.is_open())
    {
        throw std::runtime_error(std::format("Cannot open log file: {}", m_path_logfile));
    }
            
}

/* ============================================================================
|                        TintinReporter public methods                        |
============================================================================ */

void TintinReporter::log(LogLevel level, std::string_view msg)
{
    if(!m_file.is_open())
        throw std::runtime_error(std::format("Cannot open log file: {}", m_path_logfile));
    m_file << getCurrentTime() << " " << levelToString(level) << " - " << Config::DAEMON_NAME << ": "  << msg << std::endl; 
}

bool TintinReporter::isOpen() const
{
    return(m_file.is_open());
}

/* ============================================================================
|                        TintinReporter private methods                       |
============================================================================ */

std::string TintinReporter::getCurrentTime() const
{
    auto system_time = std::chrono::system_clock::now();
    std::time_t time_now = std::chrono::system_clock::to_time_t(system_time);

    std::tm tm_now;
    localtime_r(&time_now, &tm_now);

    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "[%d/%m/%Y-%H:%M:%S]", &tm_now);
    
    return std::string(buffer);
}

std::string_view TintinReporter::levelToString(LogLevel level) const
{
    switch (level)
    {
        case LogLevel::Info:    return "[ INFO ]";
        case LogLevel::Log:     return "[ LOG ]";
        case LogLevel::Warning: return "[ WARNING ]";
        case LogLevel::Error:   return "[ ERROR ]";
        default:                return "[ UNKNOWN ]";
    }
}

void TintinReporter::createLogDirectory()
{
    std::string dir_to_create;
    if (m_path_logfile == Config::LOG_FILE)
        dir_to_create = std::string(Config::LOG_DIR);
    else
    {
        size_t last_slash = m_path_logfile.find_last_of("/\\");
        if (last_slash == std::string::npos)
            return;
        dir_to_create = m_path_logfile.substr(0, last_slash);
    }
    if (access(dir_to_create.c_str(), F_OK) == 0)
        return;
    if (mkdir(dir_to_create.c_str(), 0755) != 0)
        throw std::runtime_error(std::format("Cannot create log directory: {}", dir_to_create));
}
