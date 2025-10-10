#ifndef DATA_TRANSFER_MODULE_H
#define DATA_TRANSFER_MODULE_H

#include <string>
#include <thread>
#include <functional>
#include <atomic>
#include <mutex>
#include <iostream> // For standard definitions like std::cerr

// --- Cross-Platform Socket Definitions  ---
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET SocketType;
    #define CLOSE_SOCKET closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <errno.h> // For global errno
    typedef int SocketType;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define CLOSE_SOCKET close
#endif
// ---------------------------------------------------------------------------------

/**
 * @brief Cross-platform module for file data transfer (Server and Client).
 */
class DataTransferModule {
public:
    /// Progress callback type (percentage, transferred bytes, total bytes)
    using ProgressCallback = std::function<void(int, long long, long long)>;

    DataTransferModule();
    ~DataTransferModule();

    // Server API
    bool startServer(int port);

    // Client API (Implemented)
    bool connectToServer(const std::string& ip, int port);
    bool uploadFile(const std::string& localPath, const std::string& remoteName = "");
    bool downloadFile(const std::string& remoteName, const std::string& localPath = "");
    void disconnect(); // Disconnects the client connection

    // Utility & Status
    void setProgressCallback(ProgressCallback callback);
    bool isConnected() const { return isConnected_.load(); }
    bool isTransferring() const { return isTransferring_.load(); }
    
    // Delete copy constructor and assignment operator (Good Practice)
    DataTransferModule(const DataTransferModule&) = delete;
    DataTransferModule& operator=(const DataTransferModule&) = delete;

private:
    // Constants
    static constexpr size_t BufferSize = 8192;
    static constexpr size_t MAX_FILE_SIZE = 10ULL * 1024 * 1024 * 1024; // 10 GB
    static constexpr size_t MAX_FILENAME_LENGTH = 255;
    static constexpr int SOCKET_TIMEOUT_SEC = 30; // Used for SO_RCVTIMEO/SO_SNDTIMEO

    // Server Logic
    void serverMain(int port);
    void handleClient(SocketType clientSock);
    bool processUpload(SocketType sock, const std::string& fileName, long long fileSize);
    bool processDownload(SocketType sock, const std::string& fileName);
    
    // Core Socket Operations
    bool sendAll(SocketType sock, const char* data, size_t length);
    bool recvAll(SocketType sock, char* buffer, size_t length);

    // Helpers
    std::string sanitizeFileName(const std::string& fileName);
    void notifyProgress(int percentage, long long transferred, long long total);
    bool isInitialized() const;
    int getLastError() const;
    
    // State Variables
    std::atomic<bool> isConnected_{false};     // For client connection status
    std::atomic<bool> isTransferring_{false};
    std::atomic<bool> shouldStop_{false};      // For stopping server/threads
    
    SocketType serverSocket_{INVALID_SOCKET};  // Listening socket (formerly 'socket')
    SocketType clientSocket_{INVALID_SOCKET};  // Connected socket (used by client and server handler)
    
    std::thread serverThread_;
    
    ProgressCallback progressCallback_;
    std::mutex callbackMutex_;
    
    // Winsock State (Windows only)
#ifdef _WIN32
    WSADATA wsaData_;
    bool wsaInitialized_ = false;
#endif
};

#endif // DATA_TRANSFER_MODULE_H