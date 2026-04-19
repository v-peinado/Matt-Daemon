#include "TintinReporter.hpp"
#include <ctime>
#include <chrono>
#include <unistd.h>
#include <sys/stat.h>
#include <stdexcept>
#include <dirent.h>
#include <vector>
#include <fstream>

// TintinReporter - Constructors/Destructors                         

TintinReporter::TintinReporter(const Config& cfg)
    : m_log_file(cfg.log_file)
    , m_application_name(cfg.application_name)
    , m_max_size(cfg.max_size)
    , m_max_age_days(cfg.max_age_days)
    , m_compress_after_days(cfg.compress_after_days) {
        createLogDirectory();

        m_file.open(m_log_file, std::ios::out | std::ios::app);
        if (!m_file.is_open()) {
            throw std::runtime_error("Cannot open log file: " + m_log_file);
        }
}

// TintinReporter - Public methods     

void TintinReporter::log(LogLevel level, std::string_view msg) {
    checkAndRotate();
    
    if (!m_file.is_open())
        throw std::runtime_error("Cannot open log file: " + m_log_file);
    
    m_file  << getCurrentTime() 
            << " " 
            << levelToString(level) 
            << " - " 
            << m_application_name 
            << ": "
            << msg << std::endl;
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

std::string_view TintinReporter::levelToString(LogLevel level) const {
    return m_lvl_names[static_cast<int>(level)];
}

void TintinReporter::createLogDirectory() {
    size_t last_slash = m_log_file.find_last_of("/\\");
    if (last_slash == std::string::npos)
        return;
    
    std::string dir = m_log_file.substr(0, last_slash);
    if (access(dir.c_str(), F_OK) == 0)
        return;
    
    if (mkdir(dir.c_str(), 0755) != 0)
        throw std::runtime_error("Cannot create log directory: " + dir);
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
    if (m_file.is_open()) {
        m_file.close();
    }
    
    std::string rotated_name = getRotatedFilename();
    
    if (rename(m_log_file.c_str(), rotated_name.c_str()) != 0) {
        m_file.open(m_log_file, std::ios::out | std::ios::app);
        return;
    }
    
    m_file.open(m_log_file, std::ios::out | std::ios::app);
}

std::string TintinReporter::getRotatedFilename() const {
    auto now = std::chrono::system_clock::now();
    std::time_t time_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
    localtime_r(&time_now, &tm_now);
    
    char timestamp[32];
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &tm_now);
    
    return m_log_file + "." + timestamp;
}

void TintinReporter::cleanOldLogs() {
    auto files = findLogFiles();
    
    for (const auto& file : files) {
        if (getFileAgeDays(file) > m_max_age_days) {
            unlink(file.c_str());
        }
    }
}

std::vector<std::string> TintinReporter::findLogFiles() const {
    std::vector<std::string> files;
    
    size_t last_slash = m_log_file.find_last_of("/\\");
    std::string dir_path = (last_slash != std::string::npos) 
                          ? m_log_file.substr(0, last_slash) 
                          : ".";
    
    std::string base_name = (last_slash != std::string::npos)
                           ? m_log_file.substr(last_slash + 1)
                           : m_log_file;
    
    DIR* dir = opendir(dir_path.c_str());
    if (!dir)
        return files;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        if (filename.find(base_name) == 0) {
            files.push_back(dir_path + "/" + filename);
        }
    }
    
    closedir(dir);
    return files;
}

std::size_t TintinReporter::getFileSize(const std::string& filepath) const {
    struct stat st;
    if (stat(filepath.c_str(), &st) != 0)
        return 0;
    return static_cast<std::size_t>(st.st_size);
}

int TintinReporter::getFileAgeDays(const std::string& filepath) const {
    struct stat st;
    if (stat(filepath.c_str(), &st) != 0)
        return 0;
    
    auto file_time = std::chrono::system_clock::from_time_t(st.st_mtime);
    auto now = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::hours>(now - file_time);
    
    return static_cast<int>(diff.count() / 24);
}