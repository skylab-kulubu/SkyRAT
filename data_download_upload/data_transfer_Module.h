#include "data_transfer_module.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <regex>
#include <iomanip>

namespace fs = std::filesystem;

// --- Logging and Error Definitions ---
namespace {
    std::mutex g_logMutex;
    
    void logInfo(const std::string& msg) { /* ... implementation ... */ }
    void logError(const std::string& msg) { /* ... implementation ... */ }
}

namespace {
    constexpr const char* ERR_WINSOCK_INIT = "Winsock not initialized!";
    // ... other error constants ...
    constexpr const char* CMD_UPLOAD_REQ = "UPLOAD:";
    constexpr const char* CMD_DOWNLOAD_REQ = "DOWNLOAD:";
    constexpr const char* RESP_READY = "READY";
    constexpr const char* RESP_SUCCESS = "SUCCESS:";
}

// ---------------------------------------------------------------------------------

DataTransferModule::DataTransferModule()
    : isConnected_{false}, isTransferring_{false}, shouldStop_{false},
      serverSocket_{INVALID_SOCKET}, clientSocket_{INVALID_SOCKET}
{
// ... Winsock Initialization Logic  ...
#ifdef _WIN32
    if (WSAStartup(MAKEWORD(2, 2), &wsaData_) == 0) {
        wsaInitialized_ = true;
        logInfo("Winsock initialized");
    } else {
        logError("WSAStartup failed");
    }
#endif
    logInfo("DataTransferModule initialized");
}

DataTransferModule::~DataTransferModule() {
    shouldStop_ = true;

    // Close server socket first to unblock accept()
    if (serverSocket_ != INVALID_SOCKET) {
        CLOSE_SOCKET(serverSocket_);
        serverSocket_ = INVALID_SOCKET;
    }

    if (serverThread_.joinable()) {
        serverThread_.join();
        logInfo("Server thread stopped");
    }

    disconnect(); // Close client socket if it was still open
    
// ... Winsock Cleanup Logic  ...
#ifdef _WIN32
    if (wsaInitialized_) {
        WSACleanup();
        logInfo("Winsock cleaned up");
    }
#endif
    
    logInfo("DataTransferModule destroyed");
}

void DataTransferModule::disconnect() {
    if (clientSocket_ != INVALID_SOCKET) {
        CLOSE_SOCKET(clientSocket_);
        clientSocket_ = INVALID_SOCKET;
        isConnected_ = false;
        logInfo("Client socket closed");
    }
}

// ... isInitialized(), getLastError(), startServer(), serverMain(), handleClient(), sendAll(), recvAll(), sanitizeFileName(), notifyProgress() (Logic remains largely the same, with 'socket' renamed to 'serverSocket_' where appropriate) ...

// =================================================================================
//  NEW CLIENT-SIDE IMPLEMENTATIONS 
// =================================================================================

bool DataTransferModule::connectToServer(const std::string& ip, int port) {
    if (!isInitialized()) {
        logError(ERR_WINSOCK_INIT);
        return false;
    }
    if (isConnected_.load()) {
        disconnect();
    }

    clientSocket_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket_ == INVALID_SOCKET) {
        logError(std::string("Client socket creation failed: ") + std::to_string(getLastError()));
        return false;
    }

    // Set client socket timeout (Unifying logic to use SO_RCVTIMEO/SO_SNDTIMEO)
    // Removed the redundant timeval definition from client side.

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<uint16_t>(port));

#ifdef _WIN32
    serverAddr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
    if (serverAddr.sin_addr.S_un.S_addr == INADDR_NONE) {
        logError("Invalid IP address format.");
        CLOSE_SOCKET(clientSocket_);
        clientSocket_ = INVALID_SOCKET;
        return false;
    }
#else
    if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0) {
        logError("Invalid IP address or address not supported.");
        CLOSE_SOCKET(clientSocket_);
        clientSocket_ = INVALID_SOCKET;
        return false;
    }
#endif

    if (connect(clientSocket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        logError(std::string("Connection failed to ") + ip + ":" + std::to_string(port) + 
                 " Error: " + std::to_string(getLastError()));
        CLOSE_SOCKET(clientSocket_);
        clientSocket_ = INVALID_SOCKET;
        return false;
    }

    isConnected_ = true;
    logInfo(std::string("Connected to server: ") + ip + ":" + std::to_string(port));
    return true;
}

bool DataTransferModule::uploadFile(const std::string& localPath, const std::string& remoteName) {
    if (!isConnected_.load()) {
        logError("Not connected to server.");
        return false;
    }
    if (isTransferring_.load()) {
        logError("Transfer already in progress.");
        return false;
    }

    // --- File Setup ---
    fs::path local_path(localPath);
    if (!fs::exists(local_path) || fs::is_directory(local_path)) {
        logError("Local file not found or is a directory: " + localPath);
        return false;
    }
    long long fileSize = fs::file_size(local_path);
    std::string fileName = remoteName.empty() ? local_path.filename().string() : remoteName;

    std::ifstream file(localPath, std::ios::binary);
    if (!file.is_open()) {
        logError("Failed to open local file: " + localPath);
        return false;
    }

    // --- Command Protocol ---
    std::string command = CMD_UPLOAD_REQ + fileName + ":" + std::to_string(fileSize);
    if (!sendAll(clientSocket_, command.c_str(), command.length())) {
        file.close();
        return false;
    }

    // --- Wait for READY response ---
    char response[10];
    if (recv(clientSocket_, response, 5, 0) != 5) {
        logError("Failed to receive READY signal.");
        file.close();
        return false;
    }
    response[5] = '\0';
    if (strcmp(response, RESP_READY) != 0) {
        logError(std::string("Server rejected upload: ") + response);
        file.close();
        return false;
    }
    
    // --- Transfer Data ---
    isTransferring_ = true;
    char buffer[BufferSize];
    long long totalSent = 0;
    
    while (totalSent < fileSize) {
        file.read(buffer, BufferSize);
        std::streamsize bytesRead = file.gcount();
        
        if (bytesRead == 0) break; // End of file

        if (!sendAll(clientSocket_, buffer, bytesRead)) {
            logError("Transfer interrupted during send.");
            break;
        }
        totalSent += bytesRead;
        int progress = static_cast<int>((totalSent * 100) / fileSize);
        notifyProgress(progress, totalSent, fileSize);
    }

    file.close();
    isTransferring_ = false;
    
    // --- Final Status Check ---
    char finalResponse[1024] = {0};
    int bytesReceived = recv(clientSocket_, finalResponse, 1023, 0);
    
    if (bytesReceived > 0 && std::string(finalResponse).find(RESP_SUCCESS) == 0) {
        logInfo("Upload completed successfully.");
        notifyProgress(100, fileSize, fileSize);
        return true;
    } else {
        logError(std::string("Upload failed. Server response: ") + (bytesReceived > 0 ? finalResponse : "No response"));
        return false;
    }
}

bool DataTransferModule::downloadFile(const std::string& remoteName, const std::string& localPath) {
    if (!isConnected_.load()) {
        logError("Not connected to server.");
        return false;
    }
    if (isTransferring_.load()) {
        logError("Transfer already in progress.");
        return false;
    }

    // --- File Setup ---
    std::string safeLocalPath = localPath.empty() ? ("downloaded_" + sanitizeFileName(remoteName)) : localPath;
    if (safeLocalPath.empty()) {
        logError("Invalid local path derived from remote name.");
        return false;
    }
    
    // --- Command Protocol ---
    std::string command = CMD_DOWNLOAD_REQ + remoteName;
    if (!sendAll(clientSocket_, command.c_str(), command.length())) {
        return false;
    }

    // --- Wait for DOWNLOAD:SIZE header ---
    char headerBuffer[1024] = {0};
    if (recv(clientSocket_, headerBuffer, sizeof(headerBuffer) - 1, 0) <= 0) {
        logError("Failed to receive download header.");
        return false;
    }
    std::string header(headerBuffer);
    
    if (header.find(ERR_FILE_OPEN) == 0 || header.find(ERR_INVALID_FILENAME) == 0) {
        logError(std::string("Server rejected download: ") + header);
        return false;
    }

    size_t colonPos = header.find(":");
    if (header.find(CMD_DOWNLOAD_REQ) != 0 || colonPos == std::string::npos) {
        logError("Invalid download header format.");
        return false;
    }

    long long fileSize = 0;
    try {
        fileSize = std::stoll(header.substr(colonPos + 1));
    } catch (...) {
        logError("Invalid file size in download header.");
        return false;
    }
    
    if (fileSize <= 0 || fileSize > static_cast<long long>(MAX_FILE_SIZE)) {
        logError("File size is zero, negative, or exceeds local limit.");
        return false;
    }

    // --- Open Local File ---
    std::ofstream file(safeLocalPath, std::ios::binary);
    if (!file.is_open()) {
        logError("Failed to create local file: " + safeLocalPath);
        return false;
    }
    
    // --- Transfer Data ---
    isTransferring_ = true;
    char buffer[BufferSize];
    long long totalReceived = 0;

    while (totalReceived < fileSize) {
        long long remaining = fileSize - totalReceived;
        int bytesToReceive = static_cast<int>(std::min(static_cast<long long>(BufferSize), remaining));

        int chunkSize = recv(clientSocket_, buffer, bytesToReceive, 0);
        if (chunkSize <= 0) {
            logError("Download interrupted during receive.");
            break;
        }

        file.write(buffer, chunkSize);
        if (!file.good()) {
            logError("Local file write error.");
            break;
        }
        
        totalReceived += chunkSize;
        int progress = static_cast<int>((totalReceived * 100) / fileSize);
        notifyProgress(progress, totalReceived, fileSize);
    }
    
    file.close();
    isTransferring_ = false;

    if (totalReceived == fileSize) {
        logInfo("Download completed successfully. Saved to: " + safeLocalPath);
        notifyProgress(100, fileSize, fileSize);
        return true;
    } else {
        logError(std::string("Download failed. Received ") + std::to_string(totalReceived) + 
                 " of " + std::to_string(fileSize) + " bytes.");
        fs::remove(safeLocalPath);
        return false;
    }
}

// ... processUpload(), processDownload() (Logic remains largely the same) ...