#include <iostream>
#include <winsock2.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(54000);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(server, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(server, 1);

    std::cout << "Waiting for client to connect...\n";
    SOCKET client = accept(server, nullptr, nullptr);
    std::cout << "Client connected!\n";

    while (true) {
        std::string command;
        std::cout << "Enter command (up/down/left/right/exit): ";
        std::cin >> command;

        send(client, command.c_str(), command.length() + 1, 0);

        if (command == "exit") {
            break; // bağlantıyı sonlandırır
        }
    }

    closesocket(client);
    closesocket(server);
    WSACleanup();
    return 0;
}
