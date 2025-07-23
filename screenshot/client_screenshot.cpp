// client.cpp
// This program connects to a TCP server, sends the "screenshot" command,
// then takes a screenshot and sends it to the server in PNG format.

#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <windows.h>
#include <gdiplus.h>

#pragma comment(lib, "ws2_32.lib")      // Winsock (networking) library
#pragma comment(lib, "Gdiplus.lib")     // GDI+ (graphics) library

using namespace std;
using namespace Gdiplus;

const int PORT = 4444;                  // Port used for communication with the server
const int BUFFER_SIZE = 4096;          // Buffer size used for file transfer

// Helper function to find the encoder CLSID for file formats like PNG
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0, size = 0;
    GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;

    ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (!pImageCodecInfo) return -1;

    GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }
    free(pImageCodecInfo);
    return -1;
}

// Captures the computer screen and saves it as a PNG file
void TakeScreenshot(const wstring& filename) {
    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    // Get screen resolution
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    // Create bitmap from the screen
    HDC hScreen = GetDC(nullptr);                         // Screen device context
    HDC hDC = CreateCompatibleDC(hScreen);                // Compatible memory device context
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, width, height);
    SelectObject(hDC, hBitmap);                           // Select bitmap into memory DC
    BitBlt(hDC, 0, 0, width, height, hScreen, 0, 0, SRCCOPY); // Copy screen content

    // Convert bitmap to GDI+ Bitmap object
    Bitmap bmp(hBitmap, nullptr);
    CLSID pngClsid;
    GetEncoderClsid(L"image/png", &pngClsid);
    bmp.Save(filename.c_str(), &pngClsid, nullptr);       // Save as PNG

    // Cleanup
    DeleteObject(hBitmap);
    DeleteDC(hDC);
    ReleaseDC(nullptr, hScreen);
    GdiplusShutdown(gdiplusToken);                        // Shutdown GDI+
}

// Sends the given file over the socket to the server
bool SendFile(SOCKET sock, const string& filepath) {
    ifstream file(filepath, ios::binary);                 // Open file in binary mode
    if (!file) return false;

    // Calculate file size
    file.seekg(0, ios::end);
    int fileSize = file.tellg();
    file.seekg(0, ios::beg);

    // Send file size to server first
    send(sock, (char*)&fileSize, sizeof(fileSize), 0);

    // Read and send file in chunks
    char buffer[BUFFER_SIZE];
    while (!file.eof()) {
        file.read(buffer, BUFFER_SIZE);
        int bytesRead = file.gcount();
        send(sock, buffer, bytesRead, 0);
    }
    return true;
}

int main() {
    WSADATA wsaData;
    SOCKET sock;
    sockaddr_in serverAddr = {};

    // 1. Initialize Winsock
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
        return -1;

    // 2. Create socket
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return -1;
    }

    // 3. Set server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // localhost (same machine)

    // 4. Connect to the server
    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    // 5. Send "screenshot" command
    const char* cmd = "screenshot";
    send(sock, cmd, strlen(cmd), 0);

    // 6. Take a screenshot and save to file
    wstring screenshotFile = L"screenshot.png";
    TakeScreenshot(screenshotFile);

    // 7. Send the file to the server
    if (!SendFile(sock, "screenshot.png")) {
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    // 8. Close connection and clean up resources
    closesocket(sock);
    WSACleanup();
    return 0;
}
