# Glide App - vcpkg Installation Guide

This guide explains how to build Glide App using vcpkg for dependency management, while maintaining compatibility with the existing system package manager.

## Prerequisites

### Option 1: Using vcpkg (Recommended)
1. **Install vcpkg** (if not already installed):
   ```bash
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   ./bootstrap-vcpkg.sh  # Linux/macOS
   ./bootstrap-vcpkg.bat # Windows
   ```

2. **Set VCPKG_ROOT environment variable**:
   ```bash
   export VCPKG_ROOT=/path/to/vcpkg
   ```

3. **Install required packages**:
   ```bash
   ./vcpkg install qt6-base qt6-tools openssl
   ./vcpkg install --triplet=x64-linux xtst  # Linux only
   ```

### Option 2: Using System Package Manager
Install dependencies using your system package manager:
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install qt6-base-dev qt6-tools-dev libssl-dev libx11-dev libxtst-dev

# CentOS/RHEL/Fedora
sudo dnf install qt6-qtbase-devel qt6-qttools-devel openssl-devel libX11-devel libXtst-devel
```

## Building with vcpkg

### Method 1: Automatic vcpkg Detection
```bash
# Set environment variable
export VCPKG_ROOT=/path/to/vcpkg

# Configure and build
cmake -B build
cmake --build build
```

### Method 2: Force vcpkg Usage
```bash
cmake -B build -DUSE_VCPKG=ON
cmake --build build
```

### Method 3: Specify vcpkg Toolchain
```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

## Building with System Package Manager

```bash
# Configure and build (vcpkg will be automatically disabled)
cmake -B build
cmake --build build
```

## CMake Options

| Option | Description | Default |
|--------|-------------|---------|
| `USE_VCPKG` | Force vcpkg usage | OFF |
| `CMAKE_TOOLCHAIN_FILE` | Path to vcpkg toolchain | Auto-detected |

## Package Sources

### vcpkg Packages
- `qt6-base` - Qt6 Core, Widgets, Network
- `qt6-tools` - Qt6 Linguist Tools
- `openssl` - OpenSSL encryption
- `xtst` - XTest extension (Linux)

### System Packages
- Qt6/Qt5 Development Libraries
- OpenSSL Development Libraries
- X11 Development Libraries
- XTest Development Libraries

## Troubleshooting

### vcpkg Not Found
```
Error: vcpkg toolchain file not found
```
**Solution**: Install vcpkg or set VCPKG_ROOT environment variable

### Qt Not Found
```
Error: Qt6/Qt5 not found
```
**Solution**: Install Qt development packages or use vcpkg

### X11/XTest Not Found (Linux)
```
Error: X11 or XTest libraries not found
```
**Solution**: Install X11 development packages or use vcpkg

## Project Structure

```
Glide/
├── CMakeLists.txt      # Main build configuration
├── vcpkg.json         # vcpkg package manifest
├── src/               # Source files
├── include/           # Header files
├── build/             # Build output (generated)
└── vcpkg/             # vcpkg installation (optional)
```

## Benefits of vcpkg

1. **Cross-platform**: Same dependencies on all platforms
2. **Version control**: Exact versions of all dependencies
3. **Isolation**: No conflicts with system packages
4. **Reproducible builds**: Same result on all machines

## Migration from System Packages

The CMake configuration automatically detects whether vcpkg is available and falls back to system packages if needed. No code changes are required to switch between package managers.

## Performance Notes

- vcpkg builds are generally faster for incremental builds
- System packages may have better integration with IDEs
- vcpkg provides more consistent builds across different systems
