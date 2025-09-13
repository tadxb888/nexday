#include "Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

Logger::Logger(const std::string& filename, bool enabled) 
    : logging_enabled(enabled) {
    if (logging_enabled) {
        std::filesystem::create_directories("logs");
        log_file.open("logs/" + filename, std::ios::app);
    }
}

Logger::~Logger() {
    if (log_file.is_open()) {
        log_file.close();
    }
}

std::string Logger::get_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void Logger::log(const std::string& level, const std::string& message) {
    if (!logging_enabled) return;
    
    auto timestamp = get_timestamp();
    auto log_entry = "[" + level + "] " + timestamp + " - " + message;
    
    std::cout << log_entry << std::endl;
    
    if (log_file.is_open()) {
        log_file << log_entry << std::endl;
        log_file.flush();
    }
}

void Logger::info(const std::string& message) {
    log("INFO", message);
}

void Logger::error(const std::string& message) {
    log("ERROR", message);
}

void Logger::debug(const std::string& message) {
    log("DEBUG", message);
}

void Logger::success(const std::string& message) {
    log("SUCCESS", message);
}