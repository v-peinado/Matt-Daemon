#pragma once
#include <string>
#include <fstream>
#include <array>
#include <string_view>
#include <vector>

class TintinReporter {
    public:

        struct Config {
            std::string log_file = "/var/log/tintin.log";
            std::string application_name = "Application";
            std::size_t max_size = 10 * 1024 * 1024;
            int max_age_days = 30;
            int compress_after_days = 1;
        };

        TintinReporter(const Config& cfg);
        TintinReporter() = delete;
        TintinReporter(const TintinReporter&) = delete;
        TintinReporter& operator=(const TintinReporter&) = delete;
        TintinReporter(TintinReporter&&) = delete;
        TintinReporter& operator=(TintinReporter&&) = delete;
        ~TintinReporter() = default;

        enum class LogLevel {
            Info,           // System lifecycle events
            Log,            // User input, except "quit"
            Warning,        // Non-critical issues
            Error           // Critical errors
        };

        void log(LogLevel level, std::string_view msg);
        [[nodiscard]] bool isOpen() const;

    private:

        std::string     m_log_file;
        std::string     m_application_name;
        std::size_t     m_max_size;
        int             m_max_age_days;
        int             m_compress_after_days;
        std::ofstream   m_file;

        static constexpr std::array<std::string_view, 4> m_lvl_names {"[ INFO ]", "[ LOG ]", "[ WARNING ]", "[ ERROR ]"};

        std::string getCurrentTime() const;
        std::string_view levelToString(LogLevel level) const;
        void createLogDirectory();

        // Advanced Log Archival
        void checkAndRotate();
        [[nodiscard]] bool shouldRotate() const;
        void rotateLogFile();
        std::string getRotatedFilename() const;
        void cleanOldLogs();
        std::vector<std::string> findLogFiles() const;
        std::size_t getFileSize(const std::string& filepath) const;
        [[nodiscard]] int getFileAgeDays(const std::string& filepath) const;
};