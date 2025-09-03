#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <cstdint>
#include <thread>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")

std::atomic<bool> running(true);

void sendCamera(SOCKET sock) {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Kamera acilamadi.\n";
        running = false;
        return;
    }

    while (running) {
        cv::Mat frame;
        if (!cap.read(frame)) break;

        std::vector<uchar> buf;
        cv::imencode(".jpg", frame, buf);

        uint32_t size = htonl(static_cast<uint32_t>(buf.size()));
        int sent = send(sock, (char*)&size, sizeof(size), 0);
        if (sent != sizeof(size)) break;

        sent = send(sock, reinterpret_cast<char*>(buf.data()), buf.size(), 0);
        if (sent != buf.size()) break;

        cv::waitKey(1);
    }

    cap.release();
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        std::cerr << "WSAStartup hatasi!\n";
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket olusturulamadi!\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    if (connect(sock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        std::cerr << "Servera baglanilamadi!\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::thread camThread(sendCamera, sock);

    while (running) {
        char buffer[64] = {0};
        int n = recv(sock, buffer, sizeof(buffer)-1, 0);
        if (n <= 0) break;

        std::string cmd(buffer, n);
        if (cmd == "CLOSE_CAMERA") {
            running = false;
            break;
        }
    }

    camThread.join();
    closesocket(sock);
    WSACleanup();
    return 0;
}
