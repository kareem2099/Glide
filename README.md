# Glide App

A Qt-based file transfer application with GUI and command-line tools.

## Features
- GUI interface for easy file transfers
- Command-line client and server tools  
- Multi-language support (Arabic, English, Russian)
- Cross-platform compatibility via Flatpak

## Installation

### From GitHub Releases (Recommended)
1. Go to [Releases](https://github.com/kareem2099/Glide/releases)
2. Download the latest `glide.flatpak` file
3. Install: `flatpak install glide.flatpak`
4. Run: `flatpak run org.glideapp.GlideApp`

### Prerequisites
- Flatpak installed on your system
- KDE runtime: `flatpak install flathub org.kde.Platform//6.9`

## Building from Source

### Option 1: Using vcpkg (Recommended)
```bash
# Clone the repository
git clone https://github.com/kareem2099/Glide.git
cd Glide

# Install vcpkg (if not already installed)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
cd ..

# Set vcpkg environment
export VCPKG_ROOT=$PWD/vcpkg

# Install dependencies
./vcpkg/vcpkg install qt6-base qt6-tools openssl
./vcpkg/vcpkg install --triplet=x64-linux xtst  # Linux only

# Build with vcpkg
cmake -B build
cmake --build build

# Run the application
./build/glide_gui
```

### Option 2: Using System Package Manager
```bash
# Clone the repository
git clone https://github.com/kareem2099/Glide.git
cd Glide

# Install system dependencies
sudo apt update
sudo apt install qt6-base-dev qt6-tools-dev libssl-dev libx11-dev libxtst-dev

# Build with system packages
cmake -B build
cmake --build build

# Run the application
./build/glide_gui
```

### Option 3: Build Flatpak
```bash
# Clone the repository
git clone https://github.com/kareem2099/Glide.git
cd Glide

# Build Flatpak
flatpak-builder --force-clean --repo=repo flatpak_build org.glideapp.GlideApp.yaml

# Install locally
flatpak --user remote-add --no-gpg-verify glide-repo repo
flatpak --user install glide-repo org.glideapp.GlideApp
```

## CMake Options

| Option | Description | Default |
|--------|-------------|---------|
| `USE_VCPKG` | Force vcpkg usage | OFF |
| `CMAKE_TOOLCHAIN_FILE` | Path to vcpkg toolchain | Auto-detected |

### Advanced CMake Usage

```bash
# Force vcpkg usage
cmake -B build -DUSE_VCPKG=ON

# Specify custom vcpkg path
cmake -B build -DCMAKE_TOOLCHAIN_FILE=/custom/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

# Debug build with verbose output
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=ON
```

## Dependencies

### vcpkg Packages
- `qt6-base` - Qt6 Core, Widgets, Network
- `qt6-tools` - Qt6 Linguist Tools
- `openssl` - OpenSSL encryption
- `xtst` - XTest extension (Linux)

### System Packages (Alternative)
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

### Build Errors
1. Clean build directory: `rm -rf build`
2. Reconfigure: `cmake -B build`
3. Rebuild: `cmake --build build --clean-first`

## Project Structure

```
Glide/
├── CMakeLists.txt      # Main build configuration
├── vcpkg.json         # vcpkg package manifest
├── README_vcpkg.md     # Detailed vcpkg guide
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

## Enhanced File Transfer System

### Overview
Glide now includes a significantly improved file transfer system with the following enhancements:

### 🚀 Performance Improvements
- **64KB buffer size** (vs. previous 4KB) - 16x larger buffers for better throughput
- **Intelligent compression** - Automatic zlib compression for files > 1KB
- **Progress tracking** - Real-time transfer progress with speed calculation
- **Chunked transfers** - Efficient handling of large files

### 🔒 Security & Reliability
- **SHA-256 checksums** - File integrity verification on both ends
- **Error recovery** - Robust error handling and recovery mechanisms
- **Transfer validation** - Automatic verification of successful transfers
- **Metadata preservation** - File timestamps and attributes maintained

### 📊 Features
- **Progress callbacks** - Real-time progress updates for UI integration
- **Transfer cancellation** - Ability to cancel ongoing transfers
- **Resume capability** - Resume interrupted transfers (framework ready)
- **Concurrent transfers** - Support for multiple simultaneous transfers
- **Cross-platform** - Works on Windows, Linux, and macOS

### Usage Examples

#### Basic File Transfer with Progress
```cpp
#include "file_transfer.h"

FileTransferManager transferManager;

// Progress callback
auto progressCallback = [](const FileTransferProgress& progress) {
    std::cout << "Progress: " << progress.percentage << "% ("
              << progress.bytesTransferred << "/" << progress.fileSize << " bytes)"
              << " - " << progress.bytesPerSecond << " bytes/sec"
              << " - " << progress.status << std::endl;
};

// Send file with progress tracking
bool success = transferManager.sendFile("192.168.1.100", "/path/to/file.txt", progressCallback);
```

#### Enhanced Transfer with Compression
```cpp
// Send file with compression enabled (default)
bool success = transferManager.sendFile("192.168.1.100", "/path/to/large_file.zip",
                                       progressCallback, true, false);
```

#### File Receiving
```cpp
// Receive file with progress tracking
FileTransferManager transferManager;
bool success = transferManager.receiveFile(clientSocket, "/output/directory", progressCallback);
```

#### Calculate File Checksum
```cpp
// Calculate SHA-256 checksum for file integrity verification
std::string checksum = FileTransferManager::calculateFileChecksum("/path/to/file");
```

### Server Integration

#### Enhanced Server
```cpp
#include "file_transfer_server.h"

// Start enhanced file transfer server
FileTransferServer::start_enhanced_file_transfer_server();
```

#### Progress Callback Integration
```cpp
// Example progress callback for GUI integration
auto guiProgressCallback = [this](const FileTransferProgress& progress) {
    // Update progress bar
    progressBar->setValue(progress.percentage);

    // Update speed label
    speedLabel->setText(QString("Speed: %1 KB/s").arg(progress.bytesPerSecond / 1024));

    // Update status
    statusLabel->setText(QString::fromStdString(progress.status));

    // Handle completion
    if (progress.isComplete) {
        QMessageBox::information(this, "Transfer Complete",
                               QString("File '%1' transferred successfully!")
                               .arg(QString::fromStdString(progress.filename)));
    }
};
```

### Performance Comparison

| Feature | Old System | Enhanced System |
|---------|------------|-----------------|
| Buffer Size | 4KB | 64KB |
| Compression | None | zlib (automatic) |
| Checksums | None | SHA-256 |
| Progress Tracking | None | Real-time |
| Error Recovery | Basic | Advanced |
| Transfer Speed | Baseline | 2-5x faster* |

*Performance improvements depend on file type, size, and network conditions.

### Technical Details

#### Protocol Format
The enhanced system uses a structured protocol:
```
FILE_START:filename:size:checksum:compressed:compressed_size:timestamp
```

#### Buffer Management
- **File I/O**: 64KB buffers for optimal disk performance
- **Network**: 64KB buffers for optimal network throughput
- **Compression**: 32KB internal buffers for zlib operations

#### Memory Usage
- **Small files** (< 1KB): No compression, minimal memory overhead
- **Large files**: Streaming compression, constant memory usage
- **Progress tracking**: Minimal overhead, updated every 1 second

### Migration from Legacy System

The enhanced system is backward compatible. Legacy functions are still available:

```cpp
// Legacy usage (still works)
send_file_tcp("192.168.1.100", "/path/to/file.txt");

// New enhanced usage
FileTransferManager transferManager;
transferManager.sendFile("192.168.1.100", "/path/to/file.txt", progressCallback);
```

### Dependencies

The enhanced file transfer system requires:
- **OpenSSL** - For SHA-256 checksums
- **zlib** - For compression/decompression
- **C++17** - For filesystem and string operations

These dependencies are automatically handled by vcpkg or can be installed via system package managers.

## Changelog

### [1.0.0] - 2025-10-08

#### Added
- Initial release of Glide file transfer application
- GUI interface for easy file transfers
- Command-line client and server tools
- Multi-language support (Arabic, English, Russian)
- Cross-platform compatibility via Flatpak
- Enhanced file transfer system with:
  - 64KB buffer size for improved performance
  - Intelligent compression for files > 1KB
  - Real-time progress tracking
  - SHA-256 checksums for integrity verification
  - Error recovery and transfer validation
- Device discovery features
- Clipboard synchronization
- Input injection capabilities

#### Features
- Qt6-based cross-platform application
- vcpkg dependency management
- CMake build system
- Flatpak packaging support

## Contributing

We welcome contributions from the community! Here's how you can help:

### Getting Started
1. Fork the repository
2. Clone your fork: `git clone https://github.com/your-username/Glide.git`
3. Create a feature branch: `git checkout -b feature/your-feature`

### Development Setup
```bash
cd Glide
# Install dependencies using vcpkg
export VCPKG_ROOT=$PWD/vcpkg
./vcpkg/vcpkg install qt6-base qt6-tools openssl xtst

# Build the project
cmake -B build
cmake --build build
```

### Making Changes
- Follow the existing code style
- Add tests for new features
- Update documentation as needed
- Ensure your code compiles and passes existing tests

### Submitting Changes
1. Push your changes: `git push origin feature/your-feature`
2. Create a Pull Request with a clear description of your changes
3. Wait for review and address any feedback

### Guidelines
- Keep commits small and focused
- Write clear commit messages
- Ensure compatibility with existing code
- Test your changes thoroughly

### Reporting Issues
- Use the GitHub Issues page to report bugs
- Include detailed steps to reproduce
- Provide system information (OS, Qt version, etc.)
- Attach logs or screenshots if relevant

## License

Glide is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Funding

These are supported funding model platforms

### GitHub Sponsors (primary)
github: kareem2099

### Alternative platforms
patreon: # Add Patreon username
open_collective: # Add Open Collective username
ko_fi: freerave # Ko-fi profile for the developer
tidelift: # Only for commercial dependencies
community_bridge: # Community Bridge project-name
liberapay: # Liberapay username
issuehunt: # Add IssueHunt username
otechie: # Add Otechie username

### Custom donation links (PayPal, Buy Me a Coffee, etc.)
custom: [
  "https://paypal.me/freerave1",
  "https://buymeacoffee.com/freerave"
]
