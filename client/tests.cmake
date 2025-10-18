# Test configuration for SkyRAT Client
# Add this to the main CMakeLists.txt or create a separate test target

# Test executable
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
        ws2_32      # Winsock2
        gdiplus     # GDI+ for screenshots
        gdi32       # GDI
        ole32       # OLE
    )
    
    target_compile_definitions(SkyRAT_Tests PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _WIN32_WINNT=0x0601  # Windows 7+
    )
else()
    target_link_libraries(SkyRAT_Tests PRIVATE
        pthread
    )
endif()

# Add test command
add_custom_target(run_tests
    COMMAND SkyRAT_Tests
    DEPENDS SkyRAT_Tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running SkyRAT integration and unit tests"
)