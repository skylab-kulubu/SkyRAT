#include "mouse_control.h"
#include <iostream>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

namespace mouse_control {

void moveMouse(const std::string& cmd) {
    POINT p;
    GetCursorPos(&p);
    if (cmd == "up") p.y -= 10;
    else if (cmd == "down") p.y += 10;
    else if (cmd == "left") p.x -= 10;
    else if (cmd == "right") p.x += 10;
    SetCursorPos(p.x, p.y);
}

void startMouseClient(const char* serverIp, int port) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(serverIp);

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Could not connect to server " << serverIp << ":" << port << "\n";
        WSACleanup();
        return;
    }

    char buffer[1024];
    while (true) {
        int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) break;

        std::string cmd(buffer, bytesReceived);

        if (cmd == "exit") {
            std::cout << "Exit command received. Shutting down...\n";
            break;
        }

        moveMouse(cmd);
    }

    closesocket(sock);
    WSACleanup();
}

}
