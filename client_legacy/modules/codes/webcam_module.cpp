#include "../headers/webcam_module.h"
#include <iostream>
#include <vector>
#include <cstdint>
#include <winsock2.h>
#include <chrono>
#include <thread>
#include <ctime>

Webcam_Module::Webcam_Module() : recording(false), clientSocket(INVALID_SOCKET) {
}

Webcam_Module::~Webcam_Module() {
    stopWebcam();
}

const char* Webcam_Module::name() const {
    return "Webcam Module";
}

void Webcam_Module::run() {
    std::cout << "Webcam Module initialized..." << std::endl;
}

bool Webcam_Module::startWebcam(SOCKET sock) {
    if (recording) {
        std::cout << "[Webcam] Already recording" << std::endl;
        return false;
    }
    
    clientSocket = sock;
    recording = true;
    
    // Start webcam capture thread
    webcamThread = std::thread(&Webcam_Module::captureAndSendFrames, this);
    
    std::cout << "[Webcam] Started webcam capture" << std::endl;
    return true;
}

void Webcam_Module::stopWebcam() {
    if (!recording) {
        return;
    }
    
    recording = false;
    
    // Wait for thread to finish
    if (webcamThread.joinable()) {
        webcamThread.join();
    }
    
    // Release camera
    if (cap.isOpened()) {
        cap.release();
    }
    
    std::cout << "[Webcam] Stopped webcam capture" << std::endl;
}

bool Webcam_Module::isRecording() const {
    return recording;
}

void Webcam_Module::captureAndSendFrames() {
    // Initialize camera
    cap.open(0);
    if (!cap.isOpened()) {
        std::cerr << "[Webcam] Error: Could not open camera" << std::endl;
        recording = false;
        return;
    }
    
    std::cout << "[Webcam] Camera opened successfully" << std::endl;
    
    // Set camera properties for better performance
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    cap.set(cv::CAP_PROP_FPS, 15);
    
    cv::Mat frame;
    std::vector<uchar> buffer;
    std::vector<int> compressionParams;
    compressionParams.push_back(cv::IMWRITE_JPEG_QUALITY);
    compressionParams.push_back(50); // Lower quality for faster transmission
    
    while (recording && cap.isOpened()) {
        if (!cap.read(frame) || frame.empty()) {
            std::cerr << "[Webcam] Error: Could not read frame" << std::endl;
            continue;
        }
        
        // Encode frame as JPEG
        if (!cv::imencode(".jpg", frame, buffer, compressionParams)) {
            std::cerr << "[Webcam] Error: Could not encode frame" << std::endl;
            continue;
        }
        
        try {
            // Create msgpack message with frame data
            std::map<std::string, std::string> data;
            data["type"] = "webcam_frame";
            data["data"] = base64_encode(std::vector<char>(buffer.begin(), buffer.end()));
            data["size"] = std::to_string(buffer.size());
            data["timestamp"] = std::to_string(std::time(nullptr));
            
            // Send frame via msgpack
            if (!send_message(clientSocket, data)) {
                std::cerr << "[Webcam] Error: Failed to send frame" << std::endl;
                break;
            }
            
            std::cout << "[Webcam] Sent frame (" << buffer.size() << " bytes)" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "[Webcam] Exception while sending frame: " << e.what() << std::endl;
            break;
        }
        
        // Small delay to control frame rate
        std::this_thread::sleep_for(std::chrono::milliseconds(66)); // ~15 FPS
    }
    
    cap.release();
    std::cout << "[Webcam] Capture thread finished" << std::endl;
}