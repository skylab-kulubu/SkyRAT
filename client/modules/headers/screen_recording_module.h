#pragma once
#include "module.h"
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <atomic>
#include <thread>
#include <cstdint>
#define MSGPACK_USE_BOOST 0
#define MSGPACK_NO_BOOST 1
#include <msgpack.hpp>

#pragma comment(lib, "Gdiplus.lib")

class ScreenRecording_Module : public Module {
public:
    ScreenRecording_Module();
    ~ScreenRecording_Module();
    
    const char* name() const override;
    void run() override;
    
    // Screen recording specific methods
    bool startRecording(SOCKET sock, int durationSeconds = 10, int fps = 30);
    void stopRecording();
    bool isRecording() const;

private:
    ULONG_PTR gdiplusToken;
    std::atomic<bool> recording;
    std::thread recordingThread;
    
    void InitializeGDIPlus();
    void ShutdownGDIPlus();
    bool CaptureScreenToMemory(std::vector<uint8_t>& pngData);
    bool SendFrame(SOCKET sock, const std::vector<uint8_t>& data, int frameNumber, int totalFrames);
    bool SendStreamStart(SOCKET sock, int durationSeconds, int fps, int width, int height);
    bool SendStreamEnd(SOCKET sock);
    void recordingLoop(SOCKET sock, int durationSeconds, int fps);
};
