#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <string>
#include <vector>
#include <cstdint>

#pragma comment(lib, "ws2_32.lib")

// Güvenli ve tam veri alımı
bool RecvAll(SOCKET sock, char* buffer, int size) {
    int totalReceived = 0;
    while (totalReceived < size) {
        int ret = recv(sock, buffer + totalReceived, size - totalReceived, 0);
        if (ret <= 0) {
            std::cerr << "[ERROR] recv() basarisiz veya baglanti koptu! ret=" << ret << "\n";
            return false;
        }
        totalReceived += ret;
    }
    return true;
}

// Tek frame alımı
bool ReceiveFrame(SOCKET sock, int frameNum) {
    uint32_t size = 0;
    if (!RecvAll(sock, (char*)&size, sizeof(size))) {
        std::cerr << "[ERROR] Boyut alinamadi veya baglanti koptu.\n";
        return false;
    }

    std::vector<char> buffer(size);
    if (!RecvAll(sock, buffer.data(), size)) {
        std::cerr << "[ERROR] Frame alinamadi veya baglanti koptu.\n";
        return false;
    }

    // Dosya kaydet
    char filename[64];
    sprintf(filename, "frame%04d.png", frameNum);
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "[ERROR] Dosya acilamadi: " << filename << "\n";
        return false;
    }
    file.write(buffer.data(), size);
    file.close();
    return true;
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[ERROR] WSAStartup hatasi!\n";
        return 1;
    }

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        std::cerr << "[ERROR] Socket olusturulamadi!\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(12345);

    if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[ERROR] Bind hatasi!\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    if (listen(listenSock, 1) == SOCKET_ERROR) {
        std::cerr << "[ERROR] Listen hatasi!\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    std::cout << "[INFO] Baglanti bekleniyor...\n";

    sockaddr_in clientAddr;
    int clientSize = sizeof(clientAddr);
    SOCKET clientSock = accept(listenSock, (sockaddr*)&clientAddr, &clientSize);
    if (clientSock == INVALID_SOCKET) {
        std::cerr << "[ERROR] Accept basarisiz!\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    std::cout << "[INFO] Client baglandi!\n";

    int frameNum = 0;
    while (ReceiveFrame(clientSock, frameNum)) {
        frameNum++;
    }

    std::cout << "[INFO] Baglanti kapatiliyor...\n";
    closesocket(clientSock);
    closesocket(listenSock);
    WSACleanup();

    std::cout << "[INFO] İslem tamamlandi.\n";

    // Otomatik video oluşturma
    std::cout << "[INFO] ffmpeg ile video olusturuluyor...\n";
    int ffmpegResult = system("ffmpeg -y -framerate 30 -i frame%04d.png output.mp4");
    if (ffmpegResult == 0) {
        std::cout << "[INFO] Video olusturuldu: output.mp4\n";
        // Tüm frame PNG dosyalarını sil
        std::cout << "[INFO] Frame PNG dosyalari siliniyor...\n";
        system("del /Q frame*.png");
    } else {
        std::cerr << "[ERROR] ffmpeg ile video olusturulamadi!\n";
    }

    return 0;
}
