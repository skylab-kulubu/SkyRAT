// server.cpp
// This program listens for incoming TCP connections. If the client sends the "screenshot" command,
// it then receives the PNG file and saves it as "received_screenshot.png".

#include <iostream>
#include <fstream>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib") // Link Winsock library

using namespace std;

const int PORT = 4444;            // Port on which the server listens
const int BUFFER_SIZE = 4096;     // Buffer size used for receiving data

int main() {
    WSADATA wsaData;
    SOCKET listenSock, clientSock;
    sockaddr_in serverAddr = {}, clientAddr = {};
    int clientAddrSize = sizeof(clientAddr);

    // 1. Initialize WinSock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return -1;

    // 2. Create TCP listening socket
    listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        WSACleanup();
        return -1;
    }

    // 3. Set server address (accept connections from any IP)
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;             // Accept connections from everyone
    serverAddr.sin_port = htons(PORT);                   // Convert port number to network byte order

    // 4. Bind socket to the specified IP and port
    if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(listenSock);
        WSACleanup();
        return -1;
    }

    // 5. Start listening
    listen(listenSock, 1);

    // 6. Accept client connection
    clientSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrSize);
    if (clientSock == INVALID_SOCKET) {
        closesocket(listenSock);
        WSACleanup();
        return -1;
    }

    // 7. Receive command from client
    char cmdBuf[32] = {};
    int cmdLen = recv(clientSock, cmdBuf, sizeof(cmdBuf), 0);
    if (cmdLen <= 0) {
        closesocket(clientSock);
        closesocket(listenSock);
        WSACleanup();
        return -1;
    }

    string command(cmdBuf, cmdLen); // Convert received command to string

    // 8. If command is "screenshot", receive the file
    if (command == "screenshot") {
        int fileSize = 0;

        // a) First, receive the file size
        int received = recv(clientSock, (char*)&fileSize, sizeof(fileSize), 0);
        if (received != sizeof(fileSize)) {
            // Error or connection lost
            closesocket(clientSock);
            closesocket(listenSock);
            WSACleanup();
            return -1;
        }

        // b) Open file in binary mode
        ofstream outFile("received_screenshot.png", ios::binary);

        char buffer[BUFFER_SIZE];
        int totalReceived = 0;

        // c) Receive file content in chunks and write to disk
        while (totalReceived < fileSize) {
            int bytes = recv(clientSock, buffer, BUFFER_SIZE, 0);
            if (bytes <= 0) break;                      // Stop if connection lost
            outFile.write(buffer, bytes);               // Write to file
            totalReceived += bytes;
        }

        outFile.close(); // Close the file
    }

    // 9. Close sockets and clean up Winsock
    closesocket(clientSock);
    closesocket(listenSock);
    WSACleanup();
    return 0;
}
