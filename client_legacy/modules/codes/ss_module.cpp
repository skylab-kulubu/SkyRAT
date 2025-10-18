#include "../headers/ss_module.h"
#include <iostream>
#include <windows.h>
#include <gdiplus.h>

using namespace Gdiplus;

const char* SS_Module::name() const {
    return "Screenshot Module";
}

// GDI+: Convert std::string to std::wstring
std::wstring StringAtoW(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// Function to take a screenshot and save it as a PNG file
void TakeScreenshot(const char* filename) {
    int width = GetSystemMetrics(SM_CXSCREEN); // Get screen width
    int height = GetSystemMetrics(SM_CYSCREEN); // Get screen height

    // Get device context of the full screen
    HDC hScreenDC = GetDC(NULL);
    // Create a compatible device context in memory
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    // Create a bitmap compatible with the screen
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    // Select the bitmap into the device context
    HGDIOBJ hOldBitmap = SelectObject(hMemoryDC, hBitmap);

    // Copy screen into bitmap
    BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);
    // Wrap the HBITMAP into a GDI+ Bitmap object
    Bitmap bmp(hBitmap, NULL);

    // CLSID for PNG encoder
    CLSID pngClsid;
    CLSIDFromString(L"{557CF406-1A04-11D3-9A73-0000F81EF32E}", &pngClsid);
    // Save bitmap as PNG
    bmp.Save(StringAtoW(filename).c_str(), &pngClsid, NULL);

    // Cleanup GDI objects
    SelectObject(hMemoryDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);
}

void SS_Module::run() {
    std::cout << "Screenshot Module..." << std::endl;
}
