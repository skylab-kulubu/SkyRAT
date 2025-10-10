#ifndef BASE_SOCKET_H
#define BASE_SOCKET_H

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
constexpr int INVALID_SOCKET = -1;
constexpr int SOCKET_ERROR = -1;
typedef int SOCKET;
#endif

#include <iostream>
#include <string>/n/**
 * @brief Base class for socket abstraction, cross-platform (Windows/Linux).
 */
class BaseSocket {
protected:
    SOCKET socket;              ///< Native socket handle
    bool initialized;           ///< WinSock initialization status (Windows only)

    /**
     * @brief Initializes platform-specific socket libraries (WinSock for Windows).
     * @return true if initialization succeeded, false otherwise.
     */
    bool initializeWinsock() {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
            return false;
        }
#endif
        return true;
    }

public:
    /**
     * @brief Constructor initializes platform socket libraries and sets up the socket handle.
     */
    BaseSocket() : socket(INVALID_SOCKET), initialized(false) {
        initialized = initializeWinsock();
    }

    /**
     * @brief Virtual destructor cleans up socket and WinSock resources.
     */
    virtual ~BaseSocket() {
        if (socket != INVALID_SOCKET) {
#ifdef _WIN32
            closesocket(socket);
#else
            close(socket);
#endif
        }
#ifdef _WIN32
        if (initialized) {
            WSACleanup();
        }
#endif
    }

    /**
     * @brief Returns initialization status of the socket system.
     */
    [[nodiscard]] bool isInitialized() const { return initialized; }
};

#endif // BASE_SOCKET_H