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

- **NetworkInterface** (`Network/NetworkInterface.h/cpp`): Network abstraction
- **SocketClient** (`Network/SocketClient.h/cpp`): Socket operations
- **MessageProtocol** (`Network/MessageProtocol.h/cpp`): Message serialization

### Utilities

- **Logger** (`Utils/Logger.h/cpp`): Centralized logging system
- **ThreadPool** (`Utils/ThreadPool.h/cpp`): Thread management
- **Utilities** (`Utils/Utilities.h/cpp`): Common utility functions

## Key Improvements Over Legacy Code

### 1. **Separation of Concerns**
- Each class has a single, well-defined responsibility
- No more 394-line monolithic files
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
- CMake 3.16+
- C++17 compatible compiler
- Visual Studio 2019+ or MinGW-w64

### Optional Dependencies
- OpenCV (for screen recording and webcam modules)
- msgpack (included in legacy includes)

### Build Steps

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build . --config Release

# Install (optional)
cmake --install .
```

## Migration Status

### ✅ Completed
- [x] Core framework architecture
- [x] Configuration system
- [x] Logging infrastructure
- [x] Build system (CMake)
- [x] Basic module interface

### 🚧 In Progress
- [ ] Connection management implementation
- [ ] Command processing system
- [ ] Module manager implementation

### 📅 Planned
- [ ] Individual module migrations
- [ ] Network abstraction layer
- [ ] Message protocol implementation
- [ ] Testing framework
- [ ] Documentation

## Legacy Code Transformation

### Before (Legacy)
```cpp
// Monolithic client.cpp with:
- 394 lines of mixed responsibilities
- Global variables everywhere
- Hardcoded constants
- Platform-specific code scattered
- Manual memory management
- No error handling
```

### After (New Architecture)
```cpp
// Clean, modular design with:
- Single responsibility classes
- Dependency injection
- Configuration management
- Proper error handling
- Modern C++ practices
- Cross-platform support
```

## Configuration

The client uses a configuration file (`client.conf`) to replace hardcoded values:

```ini
[Connection]
server_ip = 127.0.0.1
server_port = 4545
auto_reconnect = true

[Modules]
screenshot_format = PNG
keylogger_buffer_size = 1024
```

## Usage

```cpp
#include "Core/Client.h"

int main() {
    SkyRAT::Core::Client client;
    return client.run();
}
```

## Development Guidelines

1. **Follow RAII**: Use smart pointers and proper destructors
2. **Exception Safety**: Handle all exceptions appropriately
3. **Interface Segregation**: Keep interfaces minimal and focused
4. **Dependency Injection**: Avoid tight coupling between components
5. **Configuration**: Make behavior configurable, not hardcoded
6. **Logging**: Use the centralized Logger for all output
7. **Testing**: Write unit tests for all new components

## Contributing

When migrating legacy code:

1. Extract functionality into appropriate classes
2. Replace global variables with class members
3. Add proper error handling
4. Update build system (CMakeLists.txt)
5. Add configuration options
6. Write unit tests
7. Update documentation