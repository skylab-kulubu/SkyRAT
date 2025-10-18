#include "Client.h"
#include "ConnectionManager.h"
#include "CommandDispatcher.h"
#include "ConfigManager.h"
#include "../Modules/ModuleManager.h"
#include "../Utils/Logger.h"
#include "../Utils/Utilities.h"

#include <iostream>
#include <csignal>
#include <chrono>
#include <memory>
#include <winsock2.h>

namespace SkyRAT {
namespace Core {

    // Static instance for signal handling
    static Client* g_clientInstance = nullptr;

    // Signal handler function
    void globalSignalHandler(int signal) {
        if (g_clientInstance) {
            g_clientInstance->signalHandler(signal);
        }
    }

    Client::Client() 
        : m_running(false)
        , m_initialized(false) {
        // Set global instance for signal handling
        g_clientInstance = this;
    }

    Client::~Client() {
        if (m_running.load()) {
            shutdown();
        }
        g_clientInstance = nullptr;
    }

    bool Client::initialize() {
        if (m_initialized.load()) {
            return true;
        }

        std::cout << "Initializing SkyRAT Client..." << std::endl;

        // Initialize components in order
        if (!initializeComponents()) {
            std::cerr << "Failed to initialize core components" << std::endl;
            return false;
        }

        if (!initializeModules()) {
            std::cerr << "Failed to initialize modules" << std::endl;
            return false;
        }

        // Register signal handlers
        std::signal(SIGINT, globalSignalHandler);
        std::signal(SIGTERM, globalSignalHandler);

        m_initialized = true;
        std::cout << "SkyRAT Client initialized successfully" << std::endl;
        return true;
    }

    int Client::run() {
        if (!initialize()) {
            std::cerr << "Failed to initialize client" << std::endl;
            return 1;
        }

        std::cout << "Starting SkyRAT Client..." << std::endl;
        m_running = true;

        // Start services
        if (!startServices()) {
            std::cerr << "Failed to start services" << std::endl;
            shutdown();
            return 1;
        }

        // Main application loop (connection is now handled by ConnectionManager)
        while (m_running.load()) {
            // Sleep for a short duration to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Monitor connection status and perform periodic tasks
            if (m_connectionManager && m_logger) {
                static auto lastStatusUpdate = std::chrono::steady_clock::now();
                auto now = std::chrono::steady_clock::now();
                
                // Log status every 30 seconds
                if (std::chrono::duration_cast<std::chrono::seconds>(now - lastStatusUpdate).count() >= 30) {
                    auto state = m_connectionManager->getState();
                    auto stats = m_connectionManager->getStats();
                    
                    m_logger->info("Connection status: " + std::string(
                        state == ConnectionManager::ConnectionState::Connected ? "Connected" :
                        state == ConnectionManager::ConnectionState::Connecting ? "Connecting" :
                        state == ConnectionManager::ConnectionState::Reconnecting ? "Reconnecting" :
                        state == ConnectionManager::ConnectionState::Failed ? "Failed" : "Disconnected"
                    ) + " | Messages: " + std::to_string(stats.messagesSent) + " sent, " + 
                    std::to_string(stats.messagesReceived) + " received");
                    
                    lastStatusUpdate = now;
                }
            }
        }

        std::cout << "SkyRAT Client finished" << std::endl;
        return 0;
    }

    void Client::shutdown() {
        if (!m_running.load()) {
            return;
        }

        std::cout << "Shutting down SkyRAT Client..." << std::endl;
        m_running = false;

        stopServices();
        cleanupComponents();

        std::cout << "SkyRAT Client shutdown complete" << std::endl;
    }

    bool Client::isRunning() const {
        return m_running.load();
    }

    void Client::signalHandler(int signal) {
        std::cout << "\n[Signal] Received signal " << signal << ". Shutting down gracefully..." << std::endl;
        m_running = false;
    }

    bool Client::initializeComponents() {
        try {
            // Initialize logger first
            m_logger = std::make_shared<Utils::Logger>();
            if (!m_logger->initialize()) {
                std::cerr << "Failed to initialize logger" << std::endl;
                return false;
            }
            
            // Initialize configuration manager
            m_configManager = std::make_shared<ConfigManager>();
            
            // Try to load configuration from file
            std::string configPath = "config/skyrat_client.conf";
            
            // Ensure config directory exists
            std::string configDir = Utils::FileUtils::extractDirectory(configPath);
            if (!Utils::FileUtils::directoryExists(configDir)) {
                Utils::SystemUtils::createDirectory(configDir);
            }
            
            if (!m_configManager->loadFromFile(configPath)) {
                m_logger->info("Configuration file not found, creating default: " + configPath);
                m_configManager->createDefaultConfig(configPath);
            } else {
                m_logger->info("Loaded configuration from: " + configPath);
            }
            
            // Initialize connection manager with dependencies
            m_connectionManager = std::make_unique<ConnectionManager>(m_configManager, m_logger);
            if (!m_connectionManager->initialize()) {
                std::cerr << "Failed to initialize connection manager" << std::endl;
                return false;
            }
            
            // Initialize module manager first (needed for CommandDispatcher)
            m_moduleManager = std::make_unique<Modules::ModuleManager>();
            
            // Initialize command dispatcher with required dependencies
            m_commandDispatcher = std::make_unique<CommandDispatcher>(*m_moduleManager, *m_connectionManager);
            
            // Set up message handler for connection manager
            auto messageHandler = [this](const std::string& message, SOCKET socket) {
                if (m_commandDispatcher) {
                    m_commandDispatcher->processCommand(message, socket);
                }
            };
            m_connectionManager->setMessageHandler(messageHandler);
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Exception during component initialization: " << e.what() << std::endl;
            return false;
        }
    }

    bool Client::initializeModules() {
        try {
            // Initialize all modules (ModuleManager was created in initializeComponents)
            if (m_moduleManager && !m_moduleManager->initializeModules()) {
                std::cerr << "Failed to initialize modules" << std::endl;
                return false;
            }
            
            // TODO: Register modules here
            // This will be implemented when we migrate individual modules
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Exception during module initialization: " << e.what() << std::endl;
            return false;
        }
    }

    bool Client::startServices() {
        try {
            // Start connection manager
            if (m_connectionManager) {
                m_connectionManager->start();
                m_logger->info("Connection manager started");
            }
            
            // Additional service startup logic can be added here
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Exception during service startup: " << e.what() << std::endl;
            return false;
        }
    }

    void Client::stopServices() {
        try {
            // Stop connection manager
            if (m_connectionManager) {
                m_connectionManager->stop();
            }
            
            // Additional service shutdown logic
        } catch (const std::exception& e) {
            std::cerr << "Exception during service shutdown: " << e.what() << std::endl;
        }
    }

    void Client::cleanupComponents() {
        // Cleanup in reverse order of initialization
        m_moduleManager.reset();
        m_commandDispatcher.reset();
        m_connectionManager.reset();
        m_configManager.reset();
        m_logger.reset();
    }

    // connectionThreadMain is no longer needed as ConnectionManager handles its own threading

} // namespace Core
} // namespace SkyRAT