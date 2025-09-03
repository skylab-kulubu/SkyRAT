#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <thread>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")

std::atomic<bool> running(true);

void receiveFrames(SOCKET clientSock) {
    cv::VideoWriter writer;
    bool recording = false;

    while (running) {
        uint32_t sizeNet;
        int ret = recv(clientSock, (char*)&sizeNet, sizeof(sizeNet), 0);
        if (ret <= 0) break;

        uint32_t size = ntohl(sizeNet);
        if (size == 0) continue;

        std::vector<uchar> buf(size);
        int received = 0;
        while (received < (int)size) {
            int n = recv(clientSock, reinterpret_cast<char*>(buf.data()) + received, size - received, 0);
            if (n <= 0) {
                received = -1;
                break;
            }
            received += n;
        }
        if (received != (int)size) break;

        cv::Mat img = cv::imdecode(buf, cv::IMREAD_COLOR);
        if (!img.empty()) {
            cv::imshow("Server - Gelen Goruntu", img);

            // İlk frame geldiğinde VideoWriter aç
            if (!recording) {
                int fps = 20; // Yaklaşık FPS
                std::string filename = "kayit.avi";
                writer.open(filename,
                            cv::VideoWriter::fourcc('X','V','I','D'),
                            fps,
                            img.size(),
                            true);

                if (!writer.isOpened()) {
                    std::cerr << "VideoWriter acilamadi!\n";
                } else {
                    std::cout << "Kayit baslatildi: " << filename << std::endl;
                    recording = true;
                }
            }

            if (recording) {
                writer.write(img);
            }
        }

        // 'c' tuşuna basınca kapatma komutu gönder
        if (cv::waitKey(1) == 'c') {
            std::string closeCmd = "CLOSE_CAMERA";
            send(clientSock, closeCmd.c_str(), closeCmd.size(), 0);
            running = false;
            break;
        }
    }

    if (recording) {
        std::cout << "Kayit sonlandirildi.\n";
        writer.release();
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        std::cerr << "WSAStartup hatasi!\n";
        return 1;
    }

    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == INVALID_SOCKET) {
        std::cerr << "Socket olusturulamadi!\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5000);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind hatasi!\n";
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }

    if (listen(serverSock, 1) == SOCKET_ERROR) {
        std::cerr << "Listen hatasi!\n";
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }

    std::cout << "Sunucu hazir, baglanti bekleniyor...\n";

    sockaddr_in clientAddr{};
    int clientSize = sizeof(clientAddr);
    SOCKET clientSock = accept(serverSock, (sockaddr*)&clientAddr, &clientSize);
    if (clientSock == INVALID_SOCKET) {
        std::cerr << "Baglanti kabul edilemedi!\n";
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }

    std::cout << "Client baglandi.\n";

    std::string cmd = "OPEN_CAMERA";
    send(clientSock, cmd.c_str(), cmd.size(), 0);

    std::thread t(receiveFrames, clientSock);
    t.join();

    closesocket(clientSock);
    closesocket(serverSock);
    WSACleanup();
    return 0;
}
