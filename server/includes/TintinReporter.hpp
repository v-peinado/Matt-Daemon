#pragma once
#include <string>
#include <fstream>
#include <array>
#include <string_view>
#include <vector>
#include <filesystem>

class TintinReporter {
    public:

        struct Config {
            std::filesystem::path log_dir = "/var/log/tintin";
            std::filesystem::path log_file = "tintin.log";
            std::string application_name = "Application";
            std::size_t max_size = 10 * 1024 * 1024; // 10mb
            int max_age_days = 30;
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

        void log(LogLevel level, const std::string& msg);
        [[nodiscard]] bool isOpen() const;

    private:

        std::filesystem::path   m_log_dir;
        std::filesystem::path   m_log_file;
        std::string             m_application_name;
        std::size_t             m_max_size;
        int                     m_max_age_days;
        std::ofstream           m_file;

        static constexpr std::array<const char*, 4> m_lvl_names {"[ INFO ]", "[ LOG ]", "[ WARNING ]", "[ ERROR ]"};

        std::string getCurrentTime() const;
        std::string levelToString(LogLevel level) const;

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