#include "data_transfer_module.h"
#include <iostream>
#include <iomanip>

void onProgress(int percentage, long long transferred, long long total) {
    // Check total to prevent division by zero in the unlikely event it's 0.
    if (total > 0) {
        std::cout << "\rProgress: " << percentage << "% (" 
                  << transferred / 1024 << "KB/" << total / 1024 << "KB)" << std::flush;
    }
}

int main() {
    std::cout << "=== DATA TRANSFER MODULE TEST ===" << std::endl;
    
    std::cout << "Choose mode:\n1. Server\n2. Client\nChoice: ";
    int choice;
    if (!(std::cin >> choice)) return 1;

    DataTransferModule transfer;
    
    // FIX: Set the progress callback once, so it applies to both Server and Client transfers
    transfer.setProgressCallback(onProgress); 
    
    if (choice == 1) {
        // Server Mode
        if (transfer.startServer(8888)) {
            std::cout << "Server running on port 8888. Press Enter to stop..." << std::endl;
            // Clear the input buffer from the previous std::cin >> choice
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cin.get();
        } else {
            std::cerr << "Failed to start server." << std::endl;
        }
    } else if (choice == 2) {
        // Client Mode
        std::string ip;
        int port;
        
        std::cout << "Server IP: ";
        if (!(std::cin >> ip)) return 1;
        std::cout << "Port: ";
        if (!(std::cin >> port)) return 1;
        
        if (transfer.connectToServer(ip, port)) {
            std::cout << "Connected! Choose action:\n1. Upload\n2. Download\nChoice: ";
            if (!(std::cin >> choice)) return 1;
            
            if (choice == 1) {
                std::string filePath;
                std::cout << "Local file to upload: ";
                if (!(std::cin >> filePath)) return 1;
                
                std::cout << "\nStarting upload..." << std::endl;
                if (transfer.uploadFile(filePath)) {
                    std::cout << "\nUpload successful." << std::endl;
                } else {
                    std::cout << "\nUpload failed." << std::endl;
                }
            } else if (choice == 2) {
                std::string fileName;
                std::cout << "Remote file to download: ";
                if (!(std::cin >> fileName)) return 1;
                
                std::cout << "\nStarting download..." << std::endl;
                if (transfer.downloadFile(fileName)) {
                    std::cout << "\nDownload successful." << std::endl;
                } else {
                    std::cout << "\nDownload failed." << std::endl;
                }
            } else {
                std::cerr << "Invalid client action choice." << std::endl;
            }
            transfer.disconnect();
        } else {
            std::cerr << "Connection failed." << std::endl;
        }
    } else {
        std::cerr << "Invalid mode choice." << std::endl;
    }
    
    return 0;
}