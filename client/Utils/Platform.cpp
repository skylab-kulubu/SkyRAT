#include "Platform.h"

namespace SkyRAT {
namespace Platform {

bool initialize() {
    #ifdef SKYRAT_PLATFORM_WINDOWS
        // Initialize Winsock
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            return false;
        }
        
        // Verify Winsock version
        if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
            WSACleanup();
            return false;
        }
        
        return true;
    #else
        // Unix-like platforms don't require special initialization
        return true;
    #endif
}

void cleanup() {
    #ifdef SKYRAT_PLATFORM_WINDOWS
        // Cleanup Winsock
        WSACleanup();
    #else
        // Unix-like platforms don't require special cleanup
    #endif
}

const char* getPlatformName() {
    #ifdef SKYRAT_PLATFORM_WINDOWS
        return "Windows";
    #elif defined(SKYRAT_PLATFORM_LINUX)
        return "Linux";
    #elif defined(SKYRAT_PLATFORM_MACOS)
        return "macOS";
    #else
        return "Unknown";
    #endif
}

const char* getCompilerName() {
    #ifdef SKYRAT_COMPILER_MSVC
        return "Microsoft Visual C++";
    #elif defined(SKYRAT_COMPILER_GCC)
        return "GNU GCC";
    #elif defined(SKYRAT_COMPILER_CLANG)
        return "Clang";
    #else
        return "Unknown Compiler";
    #endif
}

} // namespace Platform
} // namespace SkyRAT