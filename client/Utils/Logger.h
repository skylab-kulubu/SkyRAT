#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <mutex>

namespace SkyRAT {
namespace Utils {

    class Logger {
    public:
        enum class LogLevel {
            DEBUG = 0,
            INFO = 1,
            WARNING = 2,  
            ERR = 3,
            CRITICAL = 4
        };

        Logger();
        ~Logger();

        bool initialize(const std::string& logFile = "");
        void setLogLevel(LogLevel level);
        void log(LogLevel level, const std::string& message);

        // Convenience methods
        void debug(const std::string& message);
        void info(const std::string& message);
        void warning(const std::string& message);
        void error(const std::string& message);
        void critical(const std::string& message);

    private:
        LogLevel m_minLevel;
        std::unique_ptr<std::ofstream> m_logFile;
        std::mutex m_logMutex;
        bool m_initialized;

        std::string formatMessage(LogLevel level, const std::string& message);
        std::string levelToString(LogLevel level);
        std::string getCurrentTimestamp();
    };

} // namespace Utils
} // namespace SkyRAT