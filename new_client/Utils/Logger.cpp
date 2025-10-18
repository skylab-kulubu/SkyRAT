#include "Logger.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>

namespace SkyRAT {
namespace Utils {

    Logger::Logger() 
        : m_minLevel(LogLevel::INFO)
        , m_initialized(false) {
    }

    Logger::~Logger() {
        if (m_logFile && m_logFile->is_open()) {
            m_logFile->close();
        }
    }

    bool Logger::initialize(const std::string& logFile) {
        std::lock_guard<std::mutex> lock(m_logMutex);

        if (!logFile.empty()) {
            m_logFile = std::make_unique<std::ofstream>(logFile, std::ios::app);
            if (!m_logFile->is_open()) {
                std::cerr << "Warning: Could not open log file: " << logFile << std::endl;
                return false;
            }
        }

        m_initialized = true;
        // Don't call info() during initialization to avoid potential deadlock
        std::cout << "[INFO] Logger initialized" << std::endl;
        return true;
    }

    void Logger::setLogLevel(LogLevel level) {
        m_minLevel = level;
    }

    void Logger::log(LogLevel level, const std::string& message) {
        if (level < m_minLevel) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_logMutex);
        
        std::string formattedMessage = formatMessage(level, message);
        
        // Always output to console
        if (level >= LogLevel::ERR) {
            std::cerr << formattedMessage << std::endl;
        } else {
            std::cout << formattedMessage << std::endl;
        }

        // Output to file if available
        if (m_logFile && m_logFile->is_open()) {
            *m_logFile << formattedMessage << std::endl;
            m_logFile->flush();
        }
    }

    void Logger::debug(const std::string& message) {
        log(LogLevel::DEBUG, message);
    }

    void Logger::info(const std::string& message) {
        log(LogLevel::INFO, message);
    }

    void Logger::warning(const std::string& message) {
        log(LogLevel::WARNING, message);
    }

    void Logger::error(const std::string& message) {
        log(LogLevel::ERR, message);
    }

    void Logger::critical(const std::string& message) {
        log(LogLevel::CRITICAL, message);
    }

    std::string Logger::formatMessage(LogLevel level, const std::string& message) {
        std::stringstream ss;
        ss << "[" << getCurrentTimestamp() << "] "
           << "[" << levelToString(level) << "] "
           << message;
        return ss.str();
    }

    std::string Logger::levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:    return "DEBUG";
            case LogLevel::INFO:     return "INFO";
            case LogLevel::WARNING:  return "WARN";
            case LogLevel::ERR:      return "ERROR";
            case LogLevel::CRITICAL: return "CRIT";
            default:                 return "UNKNOWN";
        }
    }

    std::string Logger::getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

} // namespace Utils
} // namespace SkyRAT