## Files

- `registry_functions.h` - Header file with function declarations
- `registry_functions.cpp` - Implementation file with registry operations

## Functions

### `write_regkey`
Creates or writes a registry value.

```cpp
bool write_regkey(HKEY hKeyRoot, const std::string& subKey, const std::string& valueName, const std::string& data);
```

**Parameters:**
- `hKeyRoot` - Root registry key (e.g., `HKEY_CURRENT_USER`, `HKEY_LOCAL_MACHINE`)
- `subKey` - Registry subkey path
- `valueName` - Name of the registry value
- `data` - Data to write

**Returns:** `true` on success, `false` on failure

### `remove_regkey`
Deletes a registry value.

```cpp
bool remove_regkey(HKEY hKeyRoot, const std::string& subKey, const std::string& valueName);
```

**Parameters:**
- `hKeyRoot` - Root registry key
- `subKey` - Registry subkey path
- `valueName` - Name of the registry value to delete

**Returns:** `true` on success, `false` on failure

### `modify_regkey`
Updates an existing registry value.

```cpp
bool modify_regkey(HKEY hKeyRoot, const std::string& subKey, const std::string& valueName, const std::string& newData);
```

**Parameters:**
- `hKeyRoot` - Root registry key
- `subKey` - Registry subkey path
- `valueName` - Name of the registry value to modify
- `newData` - New data to set

**Returns:** `true` on success, `false` on failure

## Usage Example

```cpp
#include "registry_functions.h"
#include <iostream>

int main() {
    std::string subKey = "Software\\MyApplication";
    std::string valueName = "Version";
    std::string data = "1.0.0";
    
    if (write_regkey(HKEY_CURRENT_USER, subKey, valueName, data)) {
        std::cout << "Registry key written successfully\n";
    }
    
    if (modify_regkey(HKEY_CURRENT_USER, subKey, valueName, "2.0.0")) {
        std::cout << "Registry key modified successfully\n";
    }
    
    if (remove_regkey(HKEY_CURRENT_USER, subKey, valueName)) {
        std::cout << "Registry key removed successfully\n";
    }
    
    return 0;
}
```

## Compilation

Compile with any C++ compiler that supports Windows API:

```bash
g++ -o registry_app main.cpp registry_functions.cpp
```

## Requirements

- Windows operating system
- C++ compiler with Windows API support
- Administrator privileges may be required for `HKEY_LOCAL_MACHINE` operations

## Common Registry Roots

- `HKEY_CURRENT_USER` - Current user settings
- `HKEY_LOCAL_MACHINE` - System-wide settings
- `HKEY_CLASSES_ROOT` - File associations and COM objects
- `HKEY_USERS` - All user profiles
- `HKEY_CURRENT_CONFIG` - Current hardware profile

## Notes

- All functions handle string registry values (REG_SZ type)
- Functions automatically handle key creation when writing
- Always check return values for error handling
- Use appropriate privileges when modifying system registry keys