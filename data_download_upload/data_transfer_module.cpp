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

// --- ASSUMED: Logging helpers (logInfo, logError) and Error constants ---

DataTransferModule::DataTransferModule()
    : isConnected_{false}, isTransferring_{false}, shouldStop_{false},
      serverSocket_{INVALID_SOCKET}, clientSocket_{INVALID_SOCKET}
{
#ifdef _WIN32
    // Winsock Initialization
    if (WSAStartup(MAKEWORD(2, 2), &wsaData_) == 0) {
        wsaInitialized_ = true;
        // logInfo("Winsock initialized");
    } else {
        // logError("WSAStartup failed");
    }
#endif
    // logInfo("DataTransferModule initialized");
}

DataTransferModule::~DataTransferModule() {
    shouldStop_ = true;

    if (serverSocket_ != INVALID_SOCKET) {
        CLOSE_SOCKET(serverSocket_);
        serverSocket_ = INVALID_SOCKET;
    }

    if (serverThread_.joinable()) {
        serverThread_.join();
    }

    disconnect(); // Close client socket if open
    
#ifdef _WIN32
    if (wsaInitialized_) {
        WSACleanup();
    }
#endif
}

// Public client disconnect function
void DataTransferModule::disconnect() {
    if (clientSocket_ != INVALID_SOCKET) {
        CLOSE_SOCKET(clientSocket_);
        clientSocket_ = INVALID_SOCKET;
        isConnected_ = false;
        // logInfo("Client socket closed");
    }
}

// --- Implementation of startServer, serverMain, handleClient, etc. (Mostly Unchanged from Revision 2) ---

// === Implementation of Client API ===

bool DataTransferModule::connectToServer(const std::string& ip, int port) {
    if (!isInitialized()) { return false; }
    if (isConnected_.load()) { disconnect(); }

    clientSocket_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket_ == INVALID_SOCKET) { return false; }

    // Setting timeout on client socket (Crucial Fix)
    int opt = 1;
#ifdef _WIN32
    DWORD timeout_ms = SOCKET_TIMEOUT_SEC * 1000;
    setsockopt(clientSocket_, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout_ms, sizeof(timeout_ms));
    setsockopt(clientSocket_, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout_ms, sizeof(timeout_ms));
#else
    struct timeval timeout;
    timeout.tv_sec = SOCKET_TIMEOUT_SEC;
    timeout.tv_usec = 0;
    setsockopt(clientSocket_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(clientSocket_, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#endif

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<uint16_t>(port));

#ifdef _WIN32
    serverAddr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
#else
    if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0) { /* ... error handling ... */ }
#endif

    if (connect(clientSocket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        CLOSE_SOCKET(clientSocket_);
        clientSocket_ = INVALID_SOCKET;
        return false;
    }

    isConnected_ = true;
    return true;
}

bool DataTransferModule::uploadFile(const std::string& localPath, const std::string& remoteName) {
    if (!isConnected_.load() || isTransferring_.load()) { return false; }
    
    // ... File existence and size checks ...
    
    std::ifstream file(localPath, std::ios::binary);
    // ... File open check ...

    // ... Command protocol (UPLOAD:name:size) and wait for READY ...
    
    // ... Transfer loop using file.read() and sendAll(clientSocket_, ...) ...
    
    file.close();
    isTransferring_ = false;
    
    // ... Final status check (receive SUCCESS/ERROR) ...
    
    return true; // or false on failure
}

bool DataTransferModule::downloadFile(const std::string& remoteName, const std::string& localPath) {
    if (!isConnected_.load() || isTransferring_.load()) { return false; }

    // ... Command protocol (DOWNLOAD:name) ...

    // ... Wait for DOWNLOAD:SIZE header and extract file size ...

    // ... Open local file for writing ...
    
    // ... Transfer loop using recv(clientSocket_, ...) and file.write() ...

    // ... Check if totalReceived == fileSize ...
    
    return true; // or false on failure
}

// --- ASSUMED: All helper functions (sendAll, recvAll, sanitizeFileName, etc.) are implemented below ---
