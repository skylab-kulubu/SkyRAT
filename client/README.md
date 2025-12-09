# SkyRAT Client - New Architecture

This directory contains the refactored SkyRAT client implementation, transforming the monolithic legacy client into a clean, modular architecture.

## Architecture Overview

### Core Components

- **Client** (`Core/Client.h/cpp`): Main application class replacing the global main() function
- **ConnectionManager** (`Core/ConnectionManager.h/cpp`): Handles all network operations
- **CommandDispatcher** (`Core/CommandDispatcher.h/cpp`): Processes and routes commands
- **ConfigManager** (`Core/ConfigManager.h/cpp`): Centralized configuration management

### Module System

- **IModule** (`Modules/Imodule.h`): Interface for all modules
- **ModuleManager** (`Modules/ModuleManager.h/cpp`): Module lifecycle management
- **Implementations/** (`Modules/Implementations/`): Individual module implementations

### Network Layer

- **MessageProtocol** (`Network/MessageProtocol.h/cpp`): Message serialization and communication protocol

### Utilities

- **Logger** (`Utils/Logger.h/cpp`): Centralized logging system
- **ThreadPool** (`Utils/ThreadPool.h/cpp`): Thread management
- **Utilities** (`Utils/Utilities.h/cpp`): Common utility functions

## Key Improvements Over Legacy Code

### 1. **Separation of Concerns**
- Each class has a single, well-defined responsibility
- Clean interfaces between components

### 2. **Configuration Management**
- Replaces hardcoded constants (SERVER_IP, SERVER_PORT, etc.)
- Runtime configurable settings
- File-based configuration with templates

### 3. **Error Handling**
- Structured exception handling
- Proper resource management (RAII)
- Graceful degradation and recovery

### 4. **Modular Architecture**
- Clean module interface (IModule)
- Dependency injection
- Easy to add/remove modules

### 5. **Modern C++ Practices**
- Smart pointers for memory management
- RAII for resource handling
- Standard library containers
- Proper threading with std::thread

## Building

### Requirements
- **CMake 3.16+**
- **Visual Studio 2019+** with MSVC v19.44+
- **C++17** compatible compiler

### Dependencies (All Included)
- **Windows Socket API** (ws2_32)
- **GDI+** for graphics operations (gdiplus, gdi32)
- **OLE32** for Windows COM support

### Build Steps

```powershell
# Create build directory
mkdir build; cd build

# Configure for Windows
cmake ..

# Build release version
cmake --build . --config Release

# Run tests
.\Release\SkyRAT_Tests.exe
# Run client
.\Release\SkyRAT_Client.exe

```

```cmd
mkdir build && cd build

cmake ..

cmake --build . --config Release

.\Release\SkyRAT_Tests.exe
.\Release\SkyRAT_Client.exe
```

---

### Cross-compilation (Unix → Windows)

#### Using build script
```bash
# Make script executable
chmod +x scripts/build-cross-mingw.sh

# Check MinGW-w64 installation
./scripts/build-cross-mingw.sh check

# Build for Windows
./scripts/build-cross-mingw.sh
```

#### Manual cross-compilation
```bash
mkdir build-mingw && cd build-mingw

cmake \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw-w64-toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    ..

make -j$(nproc)
```


### Build Outputs
- **`SkyRAT_Client.exe`** - Main client application
- **`SkyRAT_Tests.exe`** - Test suite