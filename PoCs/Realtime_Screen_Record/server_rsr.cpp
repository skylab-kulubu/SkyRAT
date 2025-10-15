#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <cstdlib>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsaData;
    SOCKET serverSock, clientSock;
    sockaddr_in serverAddr, clientAddr;
    int port = 12345;
    int clientAddrLen = sizeof(clientAddr);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSA baslatilamadi!\n";
        return 1;
    }

    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == INVALID_SOCKET) {
        std::cerr << "Socket olusturulamadi!\n";
        WSACleanup();
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind hatasi!\n";
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }

    listen(serverSock, 1);
    std::cout << "Baglanti bekleniyor...\n";

    clientSock = accept(serverSock, (sockaddr*)&clientAddr, &clientAddrLen);
    if (clientSock == INVALID_SOCKET) {
        std::cerr << "Accept hatasi!\n";
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }
    std::cout << "Client baglandi.\n";

    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    int fps = 15;

    // Padding ile frameSize hesapla
    int rowSize = ((width * 3 + 3) & ~3);
    int frameSizeExpected = rowSize * height;

    // ffplay komutu
    char ffplayCmd[512];
    sprintf(ffplayCmd,
        "ffplay -loglevel quiet -f rawvideo -pixel_format bgr24 -video_size %dx%d -framerate %d -fflags nobuffer -i -",
        width, height, fps);

    FILE* pipe = _popen(ffplayCmd, "wb");
    if (!pipe) {
    std::cerr << "ffplay baslatilamadi!\n";
    closesocket(clientSock);
    closesocket(serverSock);
    WSACleanup();
    return 1;
    }

    // ffplay penceresini gizle (sonsuz yansıma oluşmaması için)
    Sleep(1000); // ffplay açılana kadar 1 saniye bekle

    HWND ffplayWnd = FindWindowA(NULL, "ffplay"); // A = ANSI versiyonu, daha güvenli
    if (ffplayWnd) {
    ShowWindow(ffplayWnd, SW_MINIMIZE); // veya SW_HIDE ile tamamen gizle
    }


    unsigned char* buffer = (unsigned char*)malloc(frameSizeExpected);
    if (!buffer) {
        std::cerr << "Bellek ayrilamadi!\n";
        goto cleanup;
    }

    std::cout << "Canli goruntu alimi basladi. ESC ile cikis yapabilirsiniz.\n";

    while (true) {
        // ESC ile çıkış
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            std::cout << "ESC algilandi, cikis yapiliyor...\n";
            break;
        }

        // Frame boyutunu al
        int netFrameSize = 0;
        int received = recv(clientSock, reinterpret_cast<char*>(&netFrameSize), sizeof(netFrameSize), 0);
        if (received <= 0) break;

        int frameSize = ntohl(netFrameSize);
        if (frameSize != frameSizeExpected) {
            std::cerr << "Boyut uyusmazligi! Beklenen: " << frameSizeExpected
                      << " alinan: " << frameSize << "\n";
            break;
        }

        // Frame verisini al
        int totalReceived = 0;
        while (totalReceived < frameSize) {
            int n = recv(clientSock, reinterpret_cast<char*>(buffer) + totalReceived, frameSize - totalReceived, 0);
            if (n <= 0) goto cleanup;
            totalReceived += n;
        }

        // ffplay'e yaz
        fwrite(buffer, 1, frameSize, pipe);
        fflush(pipe);
    }

cleanup:
    if (buffer) free(buffer);
    if (pipe) _pclose(pipe);
    closesocket(clientSock);
    closesocket(serverSock);
    WSACleanup();

    return 0;
}
