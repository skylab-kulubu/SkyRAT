#pragma once

#include <string>
#include <vector>
#include <winsock2.h>

namespace SkyRAT {
namespace Modules {

    /**
     * @brief Interface for all SkyRAT modules
     * 
     * This interface replaces the legacy Module base class and provides
     * a clean contract for all module implementations.
     */
    class IModule {
    public:
        virtual ~IModule() = default;

        /**
         * @brief Get the module name
         * @return Module name
         */
        virtual std::string getName() const = 0;

        /**
         * @brief Get the module version
         * @return Module version string
         */
        virtual std::string getVersion() const = 0;

        /**
         * @brief Initialize the module
         * @return true if initialization was successful
         */
        virtual bool initialize() = 0;

        /**
         * @brief Shutdown the module
         */
        virtual void shutdown() = 0;

        /**
         * @brief Check if module is currently running
         * @return true if running
         */
        virtual bool isRunning() const = 0;

        /**
         * @brief Start the module operation
         * @param socket Socket for communication
         * @return true if started successfully
         */
        virtual bool start(SOCKET socket) = 0;

        /**
         * @brief Stop the module operation
         * @return true if stopped successfully
         */
        virtual bool stop() = 0;

        /**
         * @brief Process a command directed to this module
         * @param command Command string
         * @param socket Socket for communication
         * @return true if command was processed successfully
         */
        virtual bool processCommand(const std::string& command, SOCKET socket) = 0;

        /**
         * @brief Get module status information
         * @return Status string
         */
        virtual std::string getStatus() const = 0;

        /**
         * @brief Check if this module can handle the given command
         * @param command Command name to check
         * @return true if this module can handle the command
         */
        virtual bool canHandleCommand(const std::string& command) const = 0;

        /**
         * @brief Execute a command through this module
         * @param command Command name
         * @param arguments Command arguments
         * @param socket Client socket for responses
         * @return true if command was executed successfully
         */
        virtual bool executeCommand(const std::string& command, const std::vector<std::string>& arguments, SOCKET socket) = 0;

        /**
         * @brief Get list of commands supported by this module
         * @return Vector of command names
         */
        virtual std::vector<std::string> getSupportedCommands() const = 0;
    };

} // namespace Modules
} // namespace SkyRAT