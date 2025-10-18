#include "../headers/data_transfer_module_wrapper.h"
#include <iostream>
#include <map>

DataTransfer_Module::DataTransfer_Module() : isServerMode(false), serverPort(8080) {
    dataTransfer = std::make_unique<DataTransferModule>();
}

DataTransfer_Module::~DataTransfer_Module() {
    disconnectFileTransfer();
}

const char* DataTransfer_Module::name() const {
    return "Data Transfer Module";
}

void DataTransfer_Module::run() {
    std::cout << "Data Transfer Module initialized..." << std::endl;
}

bool DataTransfer_Module::startFileServer(int port) {
    serverPort = port;
    isServerMode = true;
    
    if (dataTransfer->startServer(port)) {
        std::cout << "[DataTransfer] File server started on port " << port << std::endl;
        return true;
    } else {
        std::cerr << "[DataTransfer] Failed to start file server on port " << port << std::endl;
        return false;
    }
}

bool DataTransfer_Module::connectToFileServer(const std::string& ip, int port) {
    isServerMode = false;
    
    if (dataTransfer->connectToServer(ip, port)) {
        std::cout << "[DataTransfer] Connected to file server at " << ip << ":" << port << std::endl;
        return true;
    } else {
        std::cerr << "[DataTransfer] Failed to connect to file server at " << ip << ":" << port << std::endl;
        return false;
    }
}

bool DataTransfer_Module::uploadFile(SOCKET sock, const std::string& localPath, const std::string& remoteName) {
    if (!dataTransfer->isConnected()) {
        std::cerr << "[DataTransfer] Not connected to file server" << std::endl;
        return false;
    }
    
    std::string actualRemoteName = remoteName.empty() ? localPath : remoteName;
    
    if (dataTransfer->uploadFile(localPath, actualRemoteName)) {
        std::cout << "[DataTransfer] Successfully uploaded: " << localPath << " as " << actualRemoteName << std::endl;
        
        // Send notification via msgpack
        std::map<std::string, std::string> data;
        data["type"] = "file_upload_complete";
        data["local_path"] = localPath;
        data["remote_name"] = actualRemoteName;
        data["status"] = "success";
        
        send_message(sock, data);
        return true;
    } else {
        std::cerr << "[DataTransfer] Failed to upload: " << localPath << std::endl;
        
        // Send error notification via msgpack
        std::map<std::string, std::string> data;
        data["type"] = "file_upload_complete";
        data["local_path"] = localPath;
        data["remote_name"] = actualRemoteName;
        data["status"] = "error";
        
        send_message(sock, data);
        return false;
    }
}

bool DataTransfer_Module::downloadFile(SOCKET sock, const std::string& remoteName, const std::string& localPath) {
    if (!dataTransfer->isConnected()) {
        std::cerr << "[DataTransfer] Not connected to file server" << std::endl;
        return false;
    }
    
    std::string actualLocalPath = localPath.empty() ? remoteName : localPath;
    
    if (dataTransfer->downloadFile(remoteName, actualLocalPath)) {
        std::cout << "[DataTransfer] Successfully downloaded: " << remoteName << " to " << actualLocalPath << std::endl;
        
        // Send notification via msgpack
        std::map<std::string, std::string> data;
        data["type"] = "file_download_complete";
        data["remote_name"] = remoteName;
        data["local_path"] = actualLocalPath;
        data["status"] = "success";
        
        send_message(sock, data);
        return true;
    } else {
        std::cerr << "[DataTransfer] Failed to download: " << remoteName << std::endl;
        
        // Send error notification via msgpack
        std::map<std::string, std::string> data;
        data["type"] = "file_download_complete";
        data["remote_name"] = remoteName;
        data["local_path"] = actualLocalPath;
        data["status"] = "error";
        
        send_message(sock, data);
        return false;
    }
}

void DataTransfer_Module::disconnectFileTransfer() {
    if (dataTransfer) {
        dataTransfer->disconnect();
        std::cout << "[DataTransfer] Disconnected from file server" << std::endl;
    }
}

bool DataTransfer_Module::isConnected() const {
    return dataTransfer && dataTransfer->isConnected();
}

bool DataTransfer_Module::isTransferring() const {
    return dataTransfer && dataTransfer->isTransferring();
}