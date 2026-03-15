#pragma once
#include <string>
#include <fstream>
#include <array>
#include <string_view>

class TintinReporter
{
    public:

        TintinReporter();
        explicit TintinReporter(const std::string& logfile);
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

        void log(LogLevel level, std::string_view msg);
        [[nodiscard]] bool isOpen() const;

    private:

        std::string     m_path_logfile;
        std::ofstream   m_file;
        static constexpr std::array<std::string_view, 4> m_lvl_names {"[ INFO ]", "[ LOG ]", "[ WARNING ]", "[ ERROR ]"};

        [[nodiscard]] std::string getCurrentTime() const;
        [[nodiscard]] std::string_view levelToString(LogLevel level) const;
        void createLogDirectory();
};