// client.cpp
// This program connects to a TCP server, sends a "screenshot" command,
// captures the current screen, saves it as a PNG file, and sends it to the server.

#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <windows.h>
#include <gdiplus.h>
#include <string> // For wstring_convert

#pragma comment(lib, "ws2_32.lib")      // Winsock library for networking
#pragma comment(lib, "Gdiplus.lib")     // GDI+ library for image processing

using namespace std;
using namespace Gdiplus;

const int PORT = 4444;                  // Port used to communicate with the server
const char* SERVER_IP = "127.0.0.1";    // Server IP address (localhost)
const int BUFFER_SIZE = 4096;           // Buffer size for sending the file

// Helper function to find the encoder CLSID for formats like PNG
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0, size = 0;
    GetImageEncodersSize(&num, &size); // Get the number and size of encoders
    if (size == 0) return -1;

    ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (!pImageCodecInfo) return -1;

    GetImageEncoders(num, size, pImageCodecInfo); // Retrieve encoder info
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;   // Found desired encoder
            free(pImageCodecInfo);
            return j;
        }
    }
    free(pImageCodecInfo);
    return -1; // Encoder not found
}

// Captures a screenshot of the entire screen and saves it as a PNG file
void TakeScreenshot(const char* filename) {
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr); // Initialize GDI+

    int width = GetSystemMetrics(SM_CXSCREEN);   // Screen width
    int height = GetSystemMetrics(SM_CYSCREEN);  // Screen height

    HDC hScreen = GetDC(nullptr);                // Get screen device context
    HDC hDC = CreateCompatibleDC(hScreen);       // Create compatible DC
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, width, height); // Bitmap to store screenshot
    SelectObject(hDC, hBitmap);
    BitBlt(hDC, 0, 0, width, height, hScreen, 0, 0, SRCCOPY); // Copy screen into bitmap

    Bitmap bmp(hBitmap, nullptr);                // Create GDI+ bitmap
    CLSID pngClsid;
    GetEncoderClsid(L"image/png", &pngClsid);    // Get PNG encoder

    wchar_t w_filename[256];                     // Convert filename to wide char
    mbstowcs(w_filename, filename, 256);
    bmp.Save(w_filename, &pngClsid, nullptr);    // Save as PNG

    // Cleanup
    DeleteObject(hBitmap);
    DeleteDC(hDC);
    ReleaseDC(nullptr, hScreen);
    GdiplusShutdown(gdiplusToken);
}

// Sends the specified file over the socket to the server
bool SendFile(SOCKET sock, const string& filepath) {
    ifstream file(filepath, ios::binary);        // Open file in binary mode
    if (!file) {
        cerr << "Error: Could not open file -> " << filepath << endl;
        return false;
    }

    file.seekg(0, ios::end);
    long long fileSize = file.tellg();           // Get file size
    file.seekg(0, ios::beg);

    // Send the file size to the server first
    if (send(sock, (char*)&fileSize, sizeof(fileSize), 0) != sizeof(fileSize)) {
        cerr << "Error: Could not send file size." << endl;
        return false;
    }

    char buffer[BUFFER_SIZE];
    while (!file.eof()) {
        file.read(buffer, BUFFER_SIZE);          // Read file in chunks
        int bytesRead = file.gcount();
        if (send(sock, buffer, bytesRead, 0) != bytesRead) {
            cerr << "Error: Failed while sending file data." << endl;
            return false;
        }
    }
    cout << "File sent successfully." << endl;
    return true;
}

int main() {
    WSADATA wsaData;
    SOCKET sock;
    sockaddr_in serverAddr = {};

    cout << "Starting Winsock..." << endl;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        cerr << "WSAStartup failed." << endl;
        return -1;
    }

    cout << "Creating socket..." << endl;
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        cerr << "Socket creation failed." << endl;
        WSACleanup();
        return -1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);               // Convert port to network byte order
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP); // Set server IP

    cout << "Connecting to server: " << SERVER_IP << ":" << PORT << endl;
    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Connection failed." << endl;
        closesocket(sock);
        WSACleanup();
        return -1;
    }
    cout << "Connected to server." << endl;

    const char* cmd = "screenshot";
    cout << "Sending command: '" << cmd << "'..." << endl;
    send(sock, cmd, strlen(cmd), 0); // Send "screenshot" command

    const char* screenshotFile = "screenshot.png";
    cout << "Capturing screenshot and saving as '" << screenshotFile << "'..." << endl;
    TakeScreenshot(screenshotFile); // Capture and save screenshot

    cout << "Sending screenshot file to server..." << endl;
    if (!SendFile(sock, screenshotFile)) {
        cerr << "File transfer failed." << endl;
    }

    // Clean up
    cout << "Closing connection." << endl;
    closesocket(sock);
    WSACleanup();
    return 0;
}
