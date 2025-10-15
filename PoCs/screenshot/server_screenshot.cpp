#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <string>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib") // Link the Winsock library for networking

using namespace std;

const int PORT = 12345; // Server port number to listen on

// Receives a file from the client over the socket connection and saves it locally
bool ReceiveFile(SOCKET sock, const char* filename) {
    std::ofstream file(filename, std::ios::binary); // Open file in binary mode for writing
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return false;
    }

    size_t filesize = 0;
    // Receive the file size (first 8 bytes)
    int received = recv(sock, (char*)&filesize, sizeof(filesize), 0);
    if (received != sizeof(filesize)) {
        std::cerr << "Failed to receive file size." << std::endl;
        file.close();
        return false;
    }

    size_t total = 0;
    char buffer[1024];
    // Continue receiving data until the entire file is received
    while (total < filesize) {
        int bytes = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) break; // Connection closed or error
        file.write(buffer, bytes); // Write received data to file
        total += bytes;
    }

    file.close();
    return total == filesize; // Return true if all bytes were received correctly
}

int main() {
    WSADATA wsaData;
    SOCKET listenSock, clientSock;
    sockaddr_in serverAddr = {}, clientAddr = {};
    int clientAddrSize = sizeof(clientAddr);

    WSAStartup(MAKEWORD(2, 2), &wsaData); // Initialize Winsock
    listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Create a TCP socket

    // Set up server socket information
    serverAddr.sin_family = AF_INET;             // IPv4
    serverAddr.sin_addr.s_addr = INADDR_ANY;     // Listen on all interfaces
    serverAddr.sin_port = htons(PORT);           // Port number in network byte order

    bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)); // Bind socket to IP/port
    listen(listenSock, 1); // Start listening for 1 client connection

    cout << "Waiting for connection..." << endl;
    clientSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrSize); // Accept client connection
    cout << "Client connected." << endl;

    // Send command to client
    const char* command = "screenshot";
    send(clientSock, command, strlen(command), 0); // Instruct client to take a screenshot
    cout << "Command sent to client: " << command << endl;

    // Receive screenshot file from client
    const char* filename = "client_screenshot.png";
    if (ReceiveFile(clientSock, filename)) {
        cout << "Screenshot received and saved as: " << filename << endl;
    } else {
        cout << "Error receiving the file!" << endl;
    }

    // Clean up
    closesocket(clientSock);   // Close client socket
    closesocket(listenSock);   // Close server socket
    WSACleanup();              // Clean up Winsock
    return 0;
}
