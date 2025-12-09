# Toolchain file for cross-compiling from Unix to Windows using MinGW-w64
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-toolchain.cmake ..

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Set the toolchain prefix
set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)

# Find the MinGW-w64 installation
if(APPLE)
    # On macOS, typically installed via Homebrew
    set(MINGW_ROOT /usr/local/mingw-w64)
    if(NOT EXISTS ${MINGW_ROOT})
        set(MINGW_ROOT /opt/homebrew/mingw-w64)
    endif()
    if(NOT EXISTS ${MINGW_ROOT})
        set(MINGW_ROOT /usr/local)
    endif()
elseif(UNIX)
    # On Linux, typically in /usr
    set(MINGW_ROOT /usr)
endif()

# Set compilers
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_RC_COMPILER ${TOOLCHAIN_PREFIX}-windres)

# Set tools
set(CMAKE_AR ${TOOLCHAIN_PREFIX}-ar)
set(CMAKE_STRIP ${TOOLCHAIN_PREFIX}-strip)
set(CMAKE_RANLIB ${TOOLCHAIN_PREFIX}-ranlib)

# Where to look for libraries and headers
set(CMAKE_FIND_ROOT_PATH ${MINGW_ROOT}/${TOOLCHAIN_PREFIX})

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Compiler flags for better compatibility
set(CMAKE_C_FLAGS_INIT "-static-libgcc")
set(CMAKE_CXX_FLAGS_INIT "-static-libgcc -static-libstdc++")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static-libgcc -static-libstdc++ -static")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-static-libgcc -static-libstdc++")

# Target environment settings
set(CMAKE_CROSSCOMPILING TRUE)
set(CMAKE_SYSTEM_VERSION 1)

# Define that we're cross-compiling for Windows
add_definitions(-DSKYRAT_PLATFORM_WINDOWS=1)
add_definitions(-DWIN32_LEAN_AND_MEAN)
add_definitions(-DNOMINMAX)
add_definitions(-D_WIN32_WINNT=0x0601)

# Libraries that need to be linked
set(MINGW_LIBRARIES
    ws2_32
    gdiplus
    gdi32
    ole32
    shlwapi
    advapi32
    kernel32
    user32
    shell32
)

# Function to add Windows libraries to a target
function(target_link_windows_libraries target_name)
    if(CMAKE_CROSSCOMPILING)
        target_link_libraries(${target_name} PRIVATE ${MINGW_LIBRARIES})
    endif()
endfunction()

message(STATUS "MinGW-w64 Toolchain Configuration:")
message(STATUS "  System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Processor: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  CXX Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  Find Root Path: ${CMAKE_FIND_ROOT_PATH}")
message(STATUS "  Cross Compiling: ${CMAKE_CROSSCOMPILING}")