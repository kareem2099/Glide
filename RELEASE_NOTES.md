# Release v1.0.0

**Initial release of Glide - Qt6-based file transfer application**

## What's New
- Complete cross-platform file transfer application
- Enhanced file transfer system with compression and progress tracking
- GUI interface with multi-language support (Arabic, English, Russian)
- Command-line tools for server and client operations
- Device discovery and input injection features
- Clipboard synchronization capabilities
- Flatpak packaging support for Linux distribution

## Installation
- Download and install from [GitHub Releases](https://github.com/kareem2099/Glide/releases)
- Follow the detailed build instructions in README.md

## Build Instructions
For Flatpak (Linux):
```
flatpak-builder --force-clean --repo=repo flatpak_build org.glideapp.GlideApp.yaml
flatpak build-bundle repo glide.flatpak org.glideapp.GlideApp
```

## Assets
Due to build environment limitations, Flatpak bundle not included in this release.
Please build locally using the above commands or check README.md for alternative installation methods.

## Requirements
- Qt6, OpenSSL, zlib
- CMake 3.10+
- vcpkg (recommended) or system package manager
