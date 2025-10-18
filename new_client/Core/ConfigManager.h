#pragma once

#include <string>
#include <cstdint>
#include <map>
#include <variant>

namespace SkyRAT {
namespace Core {

    /**
     * @brief Configuration management class
     * 
     * This class replaces the hardcoded constants from legacy client.cpp
     * and provides a centralized configuration system.
     */
    class ConfigManager {
    public:
        using ConfigValue = std::variant<std::string, int, uint16_t, bool, double>;

        ConfigManager();
        ~ConfigManager();

        /**
         * @brief Load configuration from file
         * @param configFile Path to configuration file
         * @return true if loaded successfully
         */
        bool loadFromFile(const std::string& configFile);

        /**
         * @brief Save configuration to file
         * @param configFile Path to configuration file
         * @return true if saved successfully
         */
        bool saveToFile(const std::string& configFile) const;

        /**
         * @brief Get configuration value
         * @param key Configuration key
         * @param defaultValue Default value if key not found
         * @return Configuration value
         */
        template<typename T>
        T getValue(const std::string& key, const T& defaultValue) const;

        /**
         * @brief Set configuration value
         * @param key Configuration key
         * @param value Configuration value
         */
        template<typename T>
        void setValue(const std::string& key, const T& value);

        // Convenience methods for common configuration values
        std::string getServerIP() const;
        uint16_t getServerPort() const;
        int getReceiveBufferSize() const;
        int getConnectionTimeout() const;
        bool getAutoReconnect() const;

        void setServerIP(const std::string& ip);
        void setServerPort(uint16_t port);
        void setReceiveBufferSize(int size);
        void setConnectionTimeout(int timeout);
        void setAutoReconnect(bool enable);

        /**
         * @brief Get reconnection interval in milliseconds
         */
        int getReconnectInterval() const;

        /**
         * @brief Get maximum reconnection attempts
         */
        int getMaxReconnectAttempts() const;

        /**
         * @brief Set reconnection interval
         */
        void setReconnectInterval(int interval);

        /**
         * @brief Set maximum reconnection attempts
         */
        void setMaxReconnectAttempts(int attempts);

        /**
         * @brief Create default configuration file
         * @param configFile Path to create configuration file
         * @return true if created successfully
         */
        bool createDefaultConfig(const std::string& configFile) const;

    private:
        std::map<std::string, ConfigValue> m_config;

        void setDefaultValues();
        void validateConfiguration();
        
        // Helper methods for configuration file parsing
        void parseAndSetValue(const std::string& key, const std::string& value);
    };

    // Template implementations
    template<typename T>
    T ConfigManager::getValue(const std::string& key, const T& defaultValue) const {
        auto it = m_config.find(key);
        if (it != m_config.end()) {
            try {
                return std::get<T>(it->second);
            } catch (const std::bad_variant_access&) {
                // Type mismatch, return default
            }
        }
        return defaultValue;
    }

    template<typename T>
    void ConfigManager::setValue(const std::string& key, const T& value) {
        m_config[key] = value;
    }

} // namespace Core
} // namespace SkyRAT