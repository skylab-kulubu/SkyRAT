// Simple cross-platform C++ client for SkyRAT server
// Works on both Windows and Linux
#include <iostream>
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

int main() {
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in server;
    char message[1024], server_reply[1024];
    int recv_size;

    // Initialize Winsock (Windows only)
#ifdef _WIN32
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }
#endif

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Could not create socket." << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    server.sin_addr.s_addr = inet_addr("127.0.0.1"); // Update with server IP
    server.sin_family = AF_INET;
    server.sin_port = htons(4444); // Update with server port

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        std::cerr << "Connection failed." << std::endl;
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return 1;
    }
    std::cout << "Connected to server." << std::endl;

    // Send a message
    strcpy(message, "Hello from client!");
    if (send(sock, message, strlen(message), 0) < 0) {
        std::cerr << "Send failed." << std::endl;
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return 1;
    }

    // Receive a reply
    recv_size = recv(sock, server_reply, sizeof(server_reply) - 1, 0);
    if (recv_size == SOCKET_ERROR) {
        std::cerr << "Receive failed." << std::endl;
    } else {
        server_reply[recv_size] = '\0';
        std::cout << "Server reply: " << server_reply << std::endl;
    }

    // Cleanup
#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
    return 0;
}
