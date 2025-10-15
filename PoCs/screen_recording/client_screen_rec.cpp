#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <cstdint>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Gdiplus.lib")

using namespace Gdiplus;

ULONG_PTR gdiplusToken;

void InitializeGDIPlus() {
    GdiplusStartupInput gdiStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiStartupInput, NULL);
}

void ShutdownGDIPlus() {
    GdiplusShutdown(gdiplusToken);
}

// PNG encoder CLSID
const CLSID CLSID_pngCodec = { 0x557CF406, 0x1A04, 0x11D3, { 0x9A, 0x73, 0x00, 0x00, 0xF8, 0x1E, 0xF3, 0x2E } };

// ekran görüntüsünü PNG olarak bellek içine kaydet
bool CaptureScreenToMemory(std::vector<BYTE>& pngData) {
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    HGDIOBJ hOldBitmap = SelectObject(hMemoryDC, hBitmap);

    BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);
    Bitmap bmp(hBitmap, NULL);

    IStream* pStream = NULL;
    CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    if (bmp.Save(pStream, &CLSID_pngCodec, NULL) != Ok) {
        pStream->Release();
        SelectObject(hMemoryDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(NULL, hScreenDC);
        std::cerr << "[ERROR] PNG kaydetme hatasi!\n";
        return false;
    }

    STATSTG statstg;
    if (pStream->Stat(&statstg, STATFLAG_NONAME) != S_OK) {
        pStream->Release();
        SelectObject(hMemoryDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(NULL, hScreenDC);
        std::cerr << "[ERROR] Stream stat hatasi!\n";
        return false;
    }

    ULONGLONG size = ((ULONGLONG)statstg.cbSize.HighPart << 32) | statstg.cbSize.LowPart;
    pngData.resize((size_t)size);

    LARGE_INTEGER liZero = {};
    pStream->Seek(liZero, STREAM_SEEK_SET, NULL);
    ULONG bytesRead = 0;
    HRESULT hr = pStream->Read(pngData.data(), (ULONG)size, &bytesRead);
    pStream->Release();

    // Temizlik
    SelectObject(hMemoryDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);

    if (hr != S_OK || bytesRead != (ULONG)size) {
        std::cerr << "[ERROR] Stream'den okuma hatasi! bytesRead=" << bytesRead << " size=" << size << "\n";
        return false;
    }
    return true;
}

// Güvenli send fonksiyonu
bool SendAll(SOCKET sock, const char* data, int size) {
    int totalSent = 0;
    while (totalSent < size) {
        int sent = send(sock, data + totalSent, size - totalSent, 0);
        if (sent <= 0) {
            std::cerr << "[ERROR] send() basarisiz! sent=" << sent << "\n";
            return false;
        }
        totalSent += sent;
    }
    return true;
}

// Frame gönderme
bool SendFrame(SOCKET sock, const std::vector<BYTE>& data) {
    uint32_t size = (uint32_t)data.size();

    // Boyutu gönder
    if (!SendAll(sock, (char*)&size, sizeof(size))) {
        std::cerr << "[ERROR] Boyut gonderilemedi.\n";
        return false;
    }

    // Veriyi gönder
    if (!SendAll(sock, (const char*)data.data(), size)) {
        std::cerr << "[ERROR] Veri gonderilemedi.\n";
        return false;
    }

    return true;
}

int main() {
    InitializeGDIPlus();

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[ERROR] WSAStartup hatasi!\n";
        ShutdownGDIPlus();
        return 1;
    }

    const char* server_ip = "127.0.0.1";
    int server_port = 12345;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "[ERROR] Socket olusturulamadi!\n";
        WSACleanup();
        ShutdownGDIPlus();
        return 1;
    }

    sockaddr_in serv_addr = {};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    serv_addr.sin_addr.s_addr = inet_addr(server_ip);

    std::cout << "[INFO] Baglaniyor: " << server_ip << ":" << server_port << "\n";

    if (connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        std::cerr << "[ERROR] Servera baglanilamadi.\n";
        closesocket(sock);
        WSACleanup();
        ShutdownGDIPlus();
        return 1;
    }

    std::cout << "[INFO] Baglanti kuruldu.\n";

    // 300 frame için döngü
    for (int frameNum = 0; frameNum < 300; ++frameNum) {
        std::vector<BYTE> pngData;
        if (!CaptureScreenToMemory(pngData)) {
            std::cerr << "[ERROR] Ekran yakalama hatasi! Frame: " << frameNum << "\n";
            break;
        }

        if (!SendFrame(sock, pngData)) {
            std::cerr << "[ERROR] Frame gönderme hatasi! Frame: " << frameNum << "\n";
            break;
        }
        Sleep(33); // 30 fps
    }

    std::cout << "[INFO] Baglanti kapatiliyor...\n";
    closesocket(sock);
    WSACleanup();
    ShutdownGDIPlus();
    return 0;
}
