#pragma once

#include "Imodule.h"
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <winsock2.h>

namespace SkyRAT {
namespace Modules {

    /**
     * @brief Central module management system
     * 
     * This class manages the lifecycle of all modules and replaces the
     * global module instances from legacy client.cpp.
     */
    class ModuleManager {
    public:
        ModuleManager();
        ~ModuleManager();

        /**
         * @brief Initialize all modules
         * @return true if all modules initialized successfully
         */
        bool initializeModules();

        /**
         * @brief Shutdown all modules
         */
        void shutdownModules();

        /**
         * @brief Register a module
         * @param module Unique pointer to module
         */
        void registerModule(std::unique_ptr<IModule> module);

        /**
         * @brief Get module by name
         * @param name Module name
         * @return Pointer to module or nullptr if not found
         */
        IModule* getModule(const std::string& name);

        /**
         * @brief Get all registered modules
         * @return Vector of module pointers
         */
        std::vector<IModule*> getAllModules();

        /**
         * @brief Check if a command can be handled by any module
         * @param command Command name to check
         * @return true if any module can handle this command
         */
        bool canHandleCommand(const std::string& command) const;

        /**
         * @brief Execute a command through the appropriate module
         * @param command Command name
         * @param arguments Command arguments
         * @param socket Client socket for responses
         * @return true if command was executed successfully
         */
        bool executeCommand(const std::string& command, const std::vector<std::string>& arguments, SOCKET socket);

        /**
         * @brief Get list of all commands supported by modules
         * @return Vector of command names
         */
        std::vector<std::string> getSupportedCommands() const;

    private:
        std::vector<std::unique_ptr<IModule>> m_modules;
        std::map<std::string, IModule*> m_modulesByName;

        void registerBuiltinModules();
    };

} // namespace Modules
} // namespace SkyRAT