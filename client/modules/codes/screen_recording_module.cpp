#include "../headers/screen_recording_module.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <windows.h> // DPIAwareness ve diğer Win32 fonksiyonları için gerekli

using namespace Gdiplus;

// PNG encoder CLSID
const CLSID CLSID_pngCodec = { 0x557CF406, 0x1A04, 0x11D3, { 0x9A, 0x73, 0x00, 0x00, 0xF8, 0x1E, 0xF3, 0x2E } };

ScreenRecording_Module::ScreenRecording_Module() : recording(false) {
    // ÇÖZÜM: Yüksek DPI (ekran ölçeklendirme) sorunu nedeniyle ekranın
    // sağ ve alt kısımlarının kesilmesini önlemek için işlemciyi DPI farkında yapar.
    // Bu, GetSystemMetrics çağrılmadan önce yapılmalıdır.
    if (!SetProcessDPIAware()) {
        std::cerr << "[UYARI] SetProcessDPIAware() çağrısı başarısız oldu.\n";
    }
    InitializeGDIPlus();
}

ScreenRecording_Module::~ScreenRecording_Module() {
    stopRecording();
    ShutdownGDIPlus();
}

const char* ScreenRecording_Module::name() const {
    return "ScreenRecording";
}

void ScreenRecording_Module::run() {
    std::cout << "[ScreenRecording] Module run() called - use startRecording() to begin recording" << std::endl;
}

void ScreenRecording_Module::InitializeGDIPlus() {
    GdiplusStartupInput gdiStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiStartupInput, NULL);
}

void ScreenRecording_Module::ShutdownGDIPlus() {
    GdiplusShutdown(gdiplusToken);
}

bool ScreenRecording_Module::CaptureScreenToMemory(std::vector<uint8_t>& pngData) {
    // SM_CXSCREEN ve SM_CYSCREEN, DPI farkındalığı sayesinde artık tam piksel boyutunu döndürmelidir.
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    HGDIOBJ hOldBitmap = SelectObject(hMemoryDC, hBitmap);

    // Ekran içeriğini belleğe kopyala
    BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);
    Bitmap bmp(hBitmap, NULL);

    IStream* pStream = NULL;
    CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    
    // GDI+ kullanarak PNG formatında akışa kaydet
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

    // Stream boyutunu hesapla ve vektörü yeniden boyutlandır
    ULONGLONG size = ((ULONGLONG)statstg.cbSize.HighPart << 32) | statstg.cbSize.LowPart;
    pngData.resize((size_t)size);

    // Stream'in başına dön ve veriyi vektöre oku
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

bool ScreenRecording_Module::SendStreamStart(SOCKET sock, int durationSeconds, int fps, int width, int height) {
    std::map<std::string, std::string> data;
    data["type"] = "screen_recording_start";
    data["duration"] = std::to_string(durationSeconds);
    data["fps"] = std::to_string(fps);
    data["width"] = std::to_string(width);
    data["height"] = std::to_string(height);
    
    return send_message(sock, data);
}

bool ScreenRecording_Module::SendStreamEnd(SOCKET sock) {
    std::map<std::string, std::string> data;
    data["type"] = "screen_recording_end";
    
    return send_message(sock, data);
}

bool ScreenRecording_Module::SendFrame(SOCKET sock, const std::vector<uint8_t>& pngData, int frameNumber, int totalFrames) {
    // Base64 kodlama ve gönderme mantığı
    std::vector<char> charData(pngData.begin(), pngData.end());
    // base64_encode() fonksiyonunun harici bir kütüphaneden geldiği varsayılır.
    std::string base64Data = base64_encode(charData); 
    
    std::map<std::string, std::string> data;
    data["type"] = "screen_frame";
    data["frame_number"] = std::to_string(frameNumber);
    data["total_frames"] = std::to_string(totalFrames);
    data["frame_data"] = base64Data;
    
    return send_message(sock, data);
}

bool ScreenRecording_Module::startRecording(SOCKET sock, int durationSeconds, int fps) {
    if (recording.load()) {
        std::cout << "[ScreenRecording] Already recording!" << std::endl;
        return false;
    }

    recording = true;
    
    // Start recording in a separate thread
    recordingThread = std::thread(&ScreenRecording_Module::recordingLoop, this, sock, durationSeconds, fps);
    
    return true;
}

void ScreenRecording_Module::stopRecording() {
    if (recording.load()) {
        recording = false;
        if (recordingThread.joinable()) {
            recordingThread.join();
        }
    }
}

bool ScreenRecording_Module::isRecording() const {
    return recording.load();
}

void ScreenRecording_Module::recordingLoop(SOCKET sock, int durationSeconds, int fps) {
    std::cout << "[ScreenRecording] Starting recording for " << durationSeconds << " seconds at " << fps << " FPS" << std::endl;
    
    int frameDelay = 1000 / fps; // milliseconds between frames
    int totalFrames = durationSeconds * fps;
    
    // Get screen dimensions
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    
    // Send stream start message
    if (!SendStreamStart(sock, durationSeconds, fps, width, height)) {
        std::cerr << "[ERROR] Failed to send stream start message" << std::endl;
        recording = false;
        return;
    }
    
    auto startTime = std::chrono::steady_clock::now();
    
    for (int frameNum = 0; frameNum < totalFrames && recording.load(); ++frameNum) {
        auto frameStart = std::chrono::steady_clock::now();
        
        std::vector<uint8_t> pngData;
        if (!CaptureScreenToMemory(pngData)) {
            std::cerr << "[ERROR] Ekran yakalama hatasi! Frame: " << frameNum << "\n";
            break;
        }

        if (!SendFrame(sock, pngData, frameNum, totalFrames)) {
            std::cerr << "[ERROR] Frame gönderme hatasi! Frame: " << frameNum << "\n";
            break;
        }
        
        // Frame timing control
        auto frameEnd = std::chrono::steady_clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
        auto sleepTime = frameDelay - frameDuration.count();
        
        if (sleepTime > 0) {
            Sleep(sleepTime);
        }
        
        if (frameNum % fps == 0) { // Every second
            std::cout << "[ScreenRecording] Recorded " << (frameNum / fps) << " seconds" << std::endl;
        }
    }
    
    // Send stream end message
    if (!SendStreamEnd(sock)) {
        std::cerr << "[ERROR] Failed to send stream end message" << std::endl;
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);
    
    std::cout << "[ScreenRecording] Recording completed. Duration: " << totalDuration.count() << " seconds" << std::endl;
    recording = false;
}
