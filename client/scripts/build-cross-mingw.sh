#!/bin/bash

# Cross-compilation build script for SkyRAT Client
# This script builds the Windows executable from Unix-like platforms using MinGW-w64

set -e  # Exit on any error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build-mingw"

echo "=== SkyRAT Client Cross-Compilation Build Script ==="
echo "Project Root: $PROJECT_ROOT"
echo "Build Directory: $BUILD_DIR"
echo ""

# Check if MinGW-w64 is installed
check_mingw() {
    echo "Checking MinGW-w64 installation..."
    
    if command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
        echo "✓ MinGW-w64 found: $(x86_64-w64-mingw32-gcc --version | head -n1)"
    else
        echo "✗ MinGW-w64 not found!"
        echo ""
        echo "Please install MinGW-w64:"
        echo ""
        if [[ "$OSTYPE" == "darwin"* ]]; then
            echo "macOS (using Homebrew):"
            echo "  brew install mingw-w64"
        elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
            echo "Ubuntu/Debian:"
            echo "  sudo apt-get install gcc-mingw-w64-x86-64 g++-mingw-w64-x86-64"
            echo ""
            echo "Fedora/CentOS:"
            echo "  sudo dnf install mingw64-gcc mingw64-gcc-c++"
            echo ""
            echo "Arch Linux:"
            echo "  sudo pacman -S mingw-w64-gcc"
        fi
        echo ""
        exit 1
    fi
}

# Clean build directory
clean_build() {
    echo "Cleaning build directory..."
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
    mkdir -p "$BUILD_DIR"
}

# Configure CMake with MinGW toolchain
configure_cmake() {
    echo "Configuring CMake with MinGW-w64 toolchain..."
    cd "$BUILD_DIR"
    
    cmake \
        -DCMAKE_TOOLCHAIN_FILE="$PROJECT_ROOT/cmake/mingw-w64-toolchain.cmake" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$BUILD_DIR/install" \
        -DBUILD_TESTING=OFF \
        "$PROJECT_ROOT"
}

# Build the project
build_project() {
    echo "Building SkyRAT Client..."
    cd "$BUILD_DIR"
    
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
}

# Package the result
package_result() {
    echo "Packaging build result..."
    cd "$BUILD_DIR"
    
    # Create package directory
    PACKAGE_DIR="$BUILD_DIR/package"
    mkdir -p "$PACKAGE_DIR"
    
    # Copy executable
    if [ -f "SkyRAT_Client.exe" ]; then
        cp "SkyRAT_Client.exe" "$PACKAGE_DIR/"
        echo "✓ Executable copied: SkyRAT_Client.exe"
    else
        echo "✗ Executable not found!"
        exit 1
    fi
    
    # Copy configuration template
    if [ -f "$PROJECT_ROOT/config/client.conf.template" ]; then
        cp "$PROJECT_ROOT/config/client.conf.template" "$PACKAGE_DIR/client.conf"
        echo "✓ Configuration template copied"
    fi
    
    # Copy README
    if [ -f "$PROJECT_ROOT/README.md" ]; then
        cp "$PROJECT_ROOT/README.md" "$PACKAGE_DIR/"
        echo "✓ README copied"
    fi
    
    echo ""
    echo "Build package created in: $PACKAGE_DIR"
    echo "Files:"
    ls -la "$PACKAGE_DIR"
}

# Main execution
main() {
    check_mingw
    clean_build
    configure_cmake
    build_project
    package_result
    
    echo ""
    echo "=== Build Completed Successfully ==="
    echo "Windows executable: $BUILD_DIR/package/SkyRAT_Client.exe"
    echo ""
    echo "To test the executable, copy it to a Windows machine or VM."
}

# Handle command line arguments
case "${1:-}" in
    clean)
        echo "Cleaning build directory only..."
        clean_build
        echo "Done."
        ;;
    check)
        check_mingw
        echo "MinGW-w64 check completed."
        ;;
    *)
        main
        ;;
esac