#include <iostream>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

void moveMouse(const std::string& cmd) {
    POINT p;
    GetCursorPos(&p);
    if (cmd == "up") p.y -= 10;
    else if (cmd == "down") p.y += 10;
    else if (cmd == "left") p.x -= 10;
    else if (cmd == "right") p.x += 10;
    SetCursorPos(p.x, p.y);
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(54000);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // veya uzak IP

    connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr));

    char buffer[1024];

    while (true) {
        int bytesReceived = recv(sock, buffer, 1024, 0);
        if (bytesReceived <= 0) break; // bağlantı kesilirse veya hata olursa döngü sona erer

        std::string cmd(buffer);

        if (cmd == "exit") {
            std::cout << "Exit command received. Shutting down...\n";
            break; // sistem kapanır veya bağlantı kesilir
        }

        moveMouse(cmd);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
