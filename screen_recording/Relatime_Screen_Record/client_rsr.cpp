#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsaData;
    SOCKET sock;
    sockaddr_in serverAddr;
    int port = 12345;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSA baslatilamadi!\n";
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket olusturulamadi!\n";
        WSACleanup();
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Sunucu IP
    serverAddr.sin_port = htons(port);

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Sunucuya baglanamadi!\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    std::cout << "Sunucuya baglandi.\n";

    // Ekran boyutu
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    int fps = 15;

    // GDI hazırlığı
    HDC hScreenDC = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    HBITMAP hOld = (HBITMAP)SelectObject(hMemDC, hBitmap);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    // ✅ Padding hesabi
    int rowSize = ((width * 3 + 3) & ~3);
    int frameSize = rowSize * height;

    unsigned char* buffer = (unsigned char*)malloc(frameSize);
    if (!buffer) {
        std::cerr << "Bellek ayrilamadi!\n";
        return 1;
    }

    auto frameDuration = std::chrono::milliseconds(1000 / fps);

    while (true) {
        auto start = std::chrono::steady_clock::now();

        // Ekrani yakala
        if (!BitBlt(hMemDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY | CAPTUREBLT))
            break;

        if (GetDIBits(hMemDC, hBitmap, 0, height, buffer, &bmi, DIB_RGB_COLORS) == 0)
            break;

        // Frame boyutunu gönder (network byte order)
        int netFrameSize = htonl(frameSize);
        int sent = send(sock, reinterpret_cast<char*>(&netFrameSize), sizeof(netFrameSize), 0);
        if (sent <= 0) break;

        // Frame verisini gönder
        int totalSent = 0;
        while (totalSent < frameSize) {
            int n = send(sock, reinterpret_cast<char*>(buffer) + totalSent, frameSize - totalSent, 0);
            if (n <= 0) goto cleanup;
            totalSent += n;
        }

        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed < frameDuration)
            std::this_thread::sleep_for(frameDuration - elapsed);
    }

cleanup:
    free(buffer);
    SelectObject(hMemDC, hOld);
    DeleteObject(hBitmap);
    DeleteDC(hMemDC);
    ReleaseDC(NULL, hScreenDC);

    closesocket(sock);
    WSACleanup();

    return 0;
}
