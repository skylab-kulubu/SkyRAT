#pragma once

/**
 * @file Platform.h
 * @brief Cross-platform abstraction layer for SkyRAT Client
 * 
 * This header provides platform-specific definitions and abstractions
 * to ensure compatibility across Windows, Linux, and macOS with different compilers.
 */

// Platform detection macros
#ifdef _WIN32
    #ifndef SKYRAT_PLATFORM_WINDOWS
        #define SKYRAT_PLATFORM_WINDOWS 1
    #endif
#elif defined(__linux__)
    #ifndef SKYRAT_PLATFORM_LINUX
        #define SKYRAT_PLATFORM_LINUX 1
    #endif
#elif defined(__APPLE__) && defined(__MACH__)
    #ifndef SKYRAT_PLATFORM_MACOS
        #define SKYRAT_PLATFORM_MACOS 1
    #endif
#endif

// Compiler detection
#if defined(_MSC_VER)
    #define SKYRAT_COMPILER_MSVC 1
#elif defined(__GNUC__)
    #define SKYRAT_COMPILER_GCC 1
#elif defined(__clang__)
    #define SKYRAT_COMPILER_CLANG 1
#endif

// Platform-specific includes and definitions
#ifdef SKYRAT_PLATFORM_WINDOWS
    // Windows platform
    #include <winsock2.h>
    #include <windows.h>
    #include <direct.h>
    #include <io.h>
    #include <shlwapi.h>
    
    // Windows-specific typedefs and macros
    #define SKYRAT_PATH_SEPARATOR '\\'
    #define SKYRAT_PATH_SEPARATOR_STR "\\"
    #define SKYRAT_MAX_PATH MAX_PATH
    #define skyrat_getcwd _getcwd
    #define skyrat_access _access
    #define skyrat_mkdir(path) _mkdir(path)
    #define skyrat_sleep(ms) Sleep(ms)
    
    // Socket compatibility
    typedef SOCKET skyrat_socket_t;
    #define SKYRAT_INVALID_SOCKET INVALID_SOCKET
    #define SKYRAT_SOCKET_ERROR SOCKET_ERROR
    #define skyrat_closesocket closesocket
    
#else
    // Unix-like platforms (Linux, macOS)
    #include <unistd.h>
    #include <sys/socket.h>
    #include <sys/stat.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <fcntl.h>
    #include <dirent.h>
    #include <errno.h>
    #include <libgen.h>
    
    // Unix-specific typedefs and macros
    #define SKYRAT_PATH_SEPARATOR '/'
    #define SKYRAT_PATH_SEPARATOR_STR "/"
    #define SKYRAT_MAX_PATH 4096
    #define skyrat_getcwd getcwd
    #define skyrat_access access
    #define skyrat_mkdir(path) mkdir(path, 0755)
    #define skyrat_sleep(ms) usleep((ms) * 1000)
    
    // Socket compatibility
    typedef int skyrat_socket_t;
    #define SKYRAT_INVALID_SOCKET (-1)
    #define SKYRAT_SOCKET_ERROR (-1)
    #define skyrat_closesocket close
    
    // Windows API compatibility layer for Unix
    #define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
    #define FILE_ATTRIBUTE_DIRECTORY 0x10
    typedef unsigned long DWORD;
    typedef void* LPSTR;
    typedef const char* LPCSTR;
    
#endif

// Compiler-specific attributes and macros
#ifdef SKYRAT_COMPILER_MSVC
    #define SKYRAT_FORCE_INLINE __forceinline
    #define SKYRAT_NOINLINE __declspec(noinline)
    #define SKYRAT_DEPRECATED __declspec(deprecated)
    #define SKYRAT_ALIGN(x) __declspec(align(x))
#elif defined(SKYRAT_COMPILER_GCC) || defined(SKYRAT_COMPILER_CLANG)
    #define SKYRAT_FORCE_INLINE __attribute__((always_inline)) inline
    #define SKYRAT_NOINLINE __attribute__((noinline))
    #define SKYRAT_DEPRECATED __attribute__((deprecated))
    #define SKYRAT_ALIGN(x) __attribute__((aligned(x)))
#else
    #define SKYRAT_FORCE_INLINE inline
    #define SKYRAT_NOINLINE
    #define SKYRAT_DEPRECATED
    #define SKYRAT_ALIGN(x)
#endif

// Cross-platform utility functions
namespace SkyRAT {
namespace Platform {

    /**
     * @brief Initialize platform-specific subsystems
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Cleanup platform-specific subsystems
     */
    void cleanup();
    
    /**
     * @brief Get platform name as string
     */
    const char* getPlatformName();
    
    /**
     * @brief Get compiler name as string
     */
    const char* getCompilerName();
    
    /**
     * @brief Check if running on Windows
     */
    constexpr bool isWindows() {
        #ifdef SKYRAT_PLATFORM_WINDOWS
            return true;
        #else
            return false;
        #endif
    }
    
    /**
     * @brief Check if running on Linux
     */
    constexpr bool isLinux() {
        #ifdef SKYRAT_PLATFORM_LINUX
            return true;
        #else
            return false;
        #endif
    }
    
    /**
     * @brief Check if running on macOS
     */
    constexpr bool isMacOS() {
        #ifdef SKYRAT_PLATFORM_MACOS
            return true;
        #else
            return false;
        #endif
    }

} // namespace Platform
} // namespace SkyRAT