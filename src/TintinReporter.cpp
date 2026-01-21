#include "TintinReporter.hpp"
#include "Config.hpp"
#include <ctime>
#include <chrono>
#include <unistd.h>
#include <sys/stat.h>
#include <stdexcept>

/* ============================================================================
|                        TintinReporter constructors                          |
============================================================================ */

TintinReporter::TintinReporter() 
    : TintinReporter(Config::LOG_FILE)
{
    // Body empty, all work in delegated constructor
}

TintinReporter::TintinReporter(const std::string& logfile)
    : m_path_logfile(logfile)
{
    if(!createLogDirectory())
    {
        throw std::runtime_error("Cannot create log directory");
    }
    m_file.open(m_path_logfile.c_str(), std::ios::out | std::ios::app);
    if(!m_file.is_open())
    {
        throw std::runtime_error("Cannot open log file: " + m_path_logfile);
    }
            
}

/* ============================================================================
|                        TintinReporter public methods                        |
============================================================================ */

void TintinReporter::log(LogLevel level, const std::string& msg)
{
    if(!m_file.is_open())
        throw std::runtime_error("Cannot open log file: " + m_path_logfile);
    std::string time = getCurrentTime();
    std::string lvlString = levelToString(level);

    m_file << time << " " << lvlString << " - " << Config::DAEMON_NAME << ": "  << msg << std::endl; 
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

std::string TintinReporter::levelToString(LogLevel level) const
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

bool TintinReporter::createLogDirectory()
{
    if(access(Config::LOG_DIR.c_str(), F_OK) == 0)
        return true;
    if (mkdir(Config::LOG_DIR.c_str(), 0755) != 0) 
        return false;
    return true;
}
