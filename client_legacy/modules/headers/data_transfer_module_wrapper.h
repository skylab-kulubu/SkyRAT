#pragma once
#include "module.h"
#include "data_transfer_module.h"
#include <memory>

class DataTransfer_Module : public Module {
private:
    std::unique_ptr<DataTransferModule> dataTransfer;
    bool isServerMode;
    int serverPort;
    
public:
    DataTransfer_Module();
    ~DataTransfer_Module();
    
    const char* name() const override;
    void run() override;
    
    // File transfer operations
    bool startFileServer(int port = 8080);
    bool connectToFileServer(const std::string& ip, int port = 8080);
    bool uploadFile(SOCKET sock, const std::string& localPath, const std::string& remoteName = "");
    bool downloadFile(SOCKET sock, const std::string& remoteName, const std::string& localPath = "");
    void disconnectFileTransfer();
    
    // Status methods
    bool isConnected() const;
    bool isTransferring() const;
};