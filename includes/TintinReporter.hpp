#pragma once
#include <string>
#include <fstream>

// File-based logger with daemon-style formatting
class TintinReporter
{
    public:

        TintinReporter();
        TintinReporter(const std::string& logfile);
        TintinReporter(const TintinReporter&) = delete;
        TintinReporter& operator=(const TintinReporter&) = delete;
        TintinReporter(TintinReporter&&) = delete;
        TintinReporter& operator=(TintinReporter&&) = delete;
        ~TintinReporter() = default;

        enum class LogLevel
        {
            Info,           // System lifecycle events
            Log,            // User input, except "quit"
            Warning,        // Non-critical issues
            Error           // Critical errors
        };

        void log(LogLevel level, const std::string& msg);
        bool isOpen() const;

    private:

        std::string     m_path_logfile;
        std::ofstream   m_file;

        std::string getCurrentTime() const;
        std::string levelToString(LogLevel level) const;
        bool createLogDirectory();
};