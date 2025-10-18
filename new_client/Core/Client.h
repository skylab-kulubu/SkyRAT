#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <string>

// Forward declarations
namespace SkyRAT {
    namespace Core {
        class ConnectionManager;
        class CommandDispatcher;
        class ConfigManager;
    }
    
    namespace Modules {
        class ModuleManager;
    }
    
    namespace Utils {
        class Logger;
    }
}

namespace SkyRAT {
namespace Core {

    /**
     * @brief Main application class that coordinates all SkyRAT client operations
     * 
     * This class replaces the monolithic main() function and global variables
     * from the legacy client.cpp, providing proper lifecycle management and
     * clean separation of concerns.
     */
    class Client {
    public:
        Client();
        ~Client();

        // Non-copyable and non-movable
        Client(const Client&) = delete;
        Client& operator=(const Client&) = delete;
        Client(Client&&) = delete;
        Client& operator=(Client&&) = delete;

        /**
         * @brief Initialize the client application
         * @return true if initialization was successful, false otherwise
         */
        bool initialize();

        /**
         * @brief Start the client application main loop
         * @return exit code (0 for success)
         */
        int run();

        /**
         * @brief Gracefully shutdown the client application
         */
        void shutdown();

        /**
         * @brief Check if the client is currently running
         * @return true if running, false otherwise
         */
        bool isRunning() const;

        /**
         * @brief Signal handler for graceful shutdown (Ctrl+C, etc.)
         */
        void signalHandler(int signal);

    private:
        // Core components (shared pointers for dependencies)
        std::shared_ptr<ConfigManager> m_configManager;
        std::unique_ptr<ConnectionManager> m_connectionManager;
        std::unique_ptr<CommandDispatcher> m_commandDispatcher;
        std::unique_ptr<Modules::ModuleManager> m_moduleManager;
        std::shared_ptr<Utils::Logger> m_logger;

        // Application state
        std::atomic<bool> m_running;
        std::atomic<bool> m_initialized;
        
        // Private methods
        bool initializeComponents();
        bool initializeModules();
        bool startServices();
        void stopServices();
        void cleanupComponents();
    };

} // namespace Core
} // namespace SkyRAT