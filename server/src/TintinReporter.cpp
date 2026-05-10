#include "TintinReporter.hpp"
#include <ctime>
#include <chrono>
#include <unistd.h>
#include <sys/stat.h>
#include <stdexcept>
#include <dirent.h>
#include <vector>
#include <fstream>
#include <filesystem>

// TintinReporter - Constructors/Destructors                         

TintinReporter::TintinReporter(const Config& cfg)
    : m_log_dir(cfg.log_dir)
    , m_log_file(cfg.log_file)
    , m_application_name(cfg.application_name)
    , m_max_size(cfg.max_size)
    , m_max_age_days(cfg.max_age_days)
{
    std::filesystem::create_directories(m_log_dir);

    m_file.open(m_log_dir / m_log_file, std::ios::out | std::ios::app);

    if (!m_file.is_open())
        throw std::runtime_error("Cannot open log file");
}

// TintinReporter - Public methods     

void TintinReporter::log(LogLevel level, const std::string& msg) {
    checkAndRotate();

    if (!m_file.is_open())
        throw std::runtime_error("Logger not open");

    m_file << getCurrentTime()
           << " "
           << levelToString(level)
           << " - "
           << m_application_name
           << ": "
           << msg
           << std::endl;
}

bool TintinReporter::isOpen() const {
    return(m_file.is_open());
}

// TintinReporter - Private methods

std::string TintinReporter::getCurrentTime() const {
    auto system_time = std::chrono::system_clock::now();
    std::time_t time_now = std::chrono::system_clock::to_time_t(system_time);

    std::tm tm_now;
    localtime_r(&time_now, &tm_now);

    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "[%d/%m/%Y-%H:%M:%S]", &tm_now);
    
    return std::string(buffer);
}

std::string TintinReporter::levelToString(LogLevel level) const {
    return std::string(m_lvl_names[static_cast<int>(level)]);
}


// Advanced Log Archival

void TintinReporter::checkAndRotate() {
    if (shouldRotate()) {
        rotateLogFile();
        cleanOldLogs();
    }
}

bool TintinReporter::shouldRotate() const {
    return getFileSize(m_log_file) >= m_max_size;
}

void TintinReporter::rotateLogFile() {
    if (m_file.is_open())
        m_file.close();

    std::string rotated_name = getRotatedFilename();

    std::filesystem::rename(m_log_dir / m_log_file, rotated_name);

    m_file.open(m_log_dir / m_log_file, std::ios::out | std::ios::app);
}

std::string TintinReporter::getRotatedFilename() const {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);

    std::tm tm;
    localtime_r(&t, &tm);

    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &tm);

    return m_log_dir / (m_log_file.string() + "." + buffer);
}

void TintinReporter::cleanOldLogs() {
    auto files = findLogFiles();
    
    for (const auto& file : files) {
        if (getFileAgeDays(file) > m_max_age_days) {
            std::filesystem::remove(file);
        }
    }
}

std::vector<std::string> TintinReporter::findLogFiles() const {
    std::vector<std::string> files;

    for (const auto& entry : std::filesystem::directory_iterator(m_log_dir)) {

        std::string name = entry.path().filename().string();

        if (name.find(m_log_file) == 0 && name != m_log_file) {
            files.push_back(entry.path().string());
        }
    }

    return files;
}

std::size_t TintinReporter::getFileSize(const std::string& filepath) const {
    return std::filesystem::file_size(filepath);
}

int TintinReporter::getFileAgeDays(const std::string& filepath) const {
    auto ftime = std::filesystem::last_write_time(filepath);

    auto now = std::filesystem::file_time_type::clock::now();
    auto diff = now - ftime;

    return std::chrono::duration_cast<std::chrono::hours>(diff).count() / 24;
}