# Glide Glide - Mouse and Keyboard Sharing

Glide Glide is a comprehensive solution for sharing mouse and keyboard input between two devices, similar to Synergy or Glide. When you move your mouse to the edge of one screen, it seamlessly transitions to the other device, allowing you to control both computers with a single mouse and keyboard.

## Features

### ✅ Core Functionality
- **Seamless Screen Transition**: Move mouse to screen edge to switch between devices
- **Bidirectional Communication**: Full UDP-based communication between server and client
- **End-to-End Encryption**: Diffie-Hellman key exchange with AES-256-CBC encryption
- **Cross-Platform Support**: Works on Windows and Linux (X11)
- **Input Synchronization**: Mouse movement, clicks, keyboard input, and scroll wheel
- **Smart Cursor Management**: Automatic cursor show/hide during transitions
- **Error Handling**: Comprehensive error handling and connection monitoring
- **Logging System**: Detailed logging for debugging and monitoring

### 🔒 Security Features
- **Diffie-Hellman Key Exchange**: Secure key establishment between devices
- **AES-256-CBC Encryption**: All input data encrypted in transit
- **Base64 Encoding**: Secure message encoding for network transmission
- **Message Authentication**: Validation of all incoming messages
- **Connection Timeout**: Automatic timeout and reconnection handling

### 🖥️ Platform Support
- **Windows**: Low-level hooks for input capture and injection
- **Linux**: X11-based input capture and XTest for input injection
- **Configurable Edge Sensitivity**: Adjustable screen edge detection
- **Multiple Monitor Support**: Works with multi-monitor setups

## Quick Start

### Prerequisites
- CMake 3.10 or higher
- C++17 compatible compiler
- OpenSSL development libraries
- Qt6 or Qt5 (for GUI components)
- Linux: X11 development libraries and XTest extension

### Building
```bash
# Clone the repository
git clone https://github.com/kareem2099/Glide.git
cd Glide

# Run the test script (builds and tests)
./test_Glide.sh

# Or build manually
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

### Usage

#### 1. Server Setup (Device with mouse/keyboard)
```bash
# Run server with client IP address
./server <CLIENT_IP_ADDRESS>

# Example:
./server 192.168.1.100
```

#### 2. Client Setup (Device to be controlled)
```bash
# Run client with server IP address
./client <SERVER_IP_ADDRESS>

# Example:
./client 192.168.1.50
```

#### 3. Using the Glide
1. **Start both applications** - Server and client will establish encrypted connection
2. **Move mouse to right edge** of server screen - cursor disappears from server
3. **Control client device** - mouse and keyboard now control the client
4. **Move mouse to left edge** of client screen - control returns to server
5. **Seamless switching** - continue moving between devices as needed

## How It Works

### Connection Establishment
1. **Server starts** and sends Diffie-Hellman parameters to client
2. **Client receives parameters** and generates its key pair
3. **Client sends public key** back to server
4. **Server completes key exchange** and establishes shared secret
5. **Encrypted communication** begins between devices

### Screen Transition Logic
```
Server Screen          Client Screen
┌─────────────┐       ┌─────────────┐
│             │       │             │
│   Server    │ ────► │   Client    │
│  (Active)   │       │ (Receives)  │
│             │       │             │
└─────────────┘       └─────────────┘
      ▲                       │
      │                       │
      └───────────────────────┘
       Mouse at left edge
```

### Message Flow
1. **Input Capture**: Server captures mouse/keyboard events
2. **Encryption**: Messages encrypted with AES-256-CBC
3. **Network Transmission**: Encrypted data sent via UDP
4. **Decryption**: Client decrypts received messages
5. **Input Injection**: Client injects input events locally

## Configuration

### Edge Sensitivity
Modify `constants.h` to adjust edge detection:
```cpp
namespace ScreenTransition {
    const int DEFAULT_EDGE_SENSITIVITY = 5; // pixels from edge
    const int DEFAULT_DEAD_ZONE = 10; // pixels
    const int DEFAULT_TRANSITION_DELAY = 100; // milliseconds
}
```

### Network Settings
```cpp
namespace Network {
    const int UDP_PORT = 45454; // Default port
    const int BUFFER_SIZE = 4096; // Message buffer size
    const int HEARTBEAT_INTERVAL = 5000; // milliseconds
}
```

## Troubleshooting

### Common Issues

#### Connection Problems
```bash
# Check if port is available
netstat -an | grep 45454

# Test network connectivity
ping <target_ip>

# Check firewall settings
sudo ufw status
```

#### Permission Issues (Linux)
```bash
# Add user to input group
sudo usermod -a -G input $USER

# Grant X11 permissions
xhost +local:

# For XTest extension
sudo apt-get install libxtst-dev
```

#### Build Issues
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install build-essential cmake libssl-dev libx11-dev libxtst-dev qt6-base-dev

# Install dependencies (CentOS/RHEL)
sudo yum install gcc-c++ cmake openssl-devel libX11-devel libXtst-devel qt6-qtbase-devel
```

### Debug Mode
Enable detailed logging by setting debug level:
```cpp
// In constants.h
namespace LogTypes {
    const std::string DEBUG = "DEBUG"; // Enable debug messages
}
```

### Performance Optimization
- **Reduce Edge Sensitivity**: Lower values for faster transitions
- **Adjust Buffer Size**: Increase for high-frequency input
- **Network Optimization**: Use wired connection for best performance

## Security Considerations

### Network Security
- **Use Private Networks**: Run on trusted local networks only
- **Firewall Configuration**: Restrict access to Glide port (45454)
- **VPN Recommended**: For connections over public networks

### Encryption Details
- **Key Exchange**: 2048-bit Diffie-Hellman parameters
- **Symmetric Encryption**: AES-256-CBC with random IV
- **Message Integrity**: Built-in validation and error detection

## Advanced Usage

### Multiple Clients
Currently supports one server and one client. For multiple clients:
1. Run separate server instances on different ports
2. Configure each client to connect to specific server port

### Custom Key Mappings
Modify `client_input_injection.cpp` to customize key mappings:
```cpp
// Example: Map Windows key codes to Linux key symbols
if (keyCode == VK_LWIN) {
    key_sym = XK_Super_L;
}
```

### Clipboard Integration
The system includes clipboard synchronization (see `clipboard_sync_manager.cpp`):
- Automatic clipboard sharing between devices
- Support for text and basic data types
- Encrypted clipboard data transmission

## API Reference

### Core Classes

#### `EncryptionManager`
- `generateDhParams()`: Generate Diffie-Hellman parameters
- `generateDhKeyPair()`: Create public/private key pair
- `computeSharedSecret()`: Establish shared encryption key
- `encrypt(message)`: Encrypt outgoing messages
- `decrypt(data)`: Decrypt incoming messages

#### `Utils` Namespace
- `sendSecureMessage()`: Send encrypted message over network
- `base64_encode()`: Encode binary data to base64
- `base64_decode()`: Decode base64 to binary data
- `translateCoordinates()`: Convert coordinates between screen sizes

### Message Types
```cpp
// Screen transition
SCREEN_EXIT:  // Server mouse left screen
SCREEN_ENTER: // Server mouse returned to screen

// Input events
KEY_PRESS:    // Keyboard key pressed
KEY_RELEASE:  // Keyboard key released
MOUSE_MOVE:   // Mouse movement
MOUSE_PRESS:  // Mouse button pressed
MOUSE_RELEASE:// Mouse button released
MOUSE_SCROLL: // Mouse wheel scroll

// Protocol messages
HEARTBEAT:    // Connection keepalive
DH_PARAMS:    // Diffie-Hellman parameters
DH_PUBLIC_KEY:// Public key exchange
ENCRYPTED_DATA:// Encrypted message payload
```

## Contributing

### Development Setup
1. Fork the repository
2. Create feature branch: `git checkout -b feature/new-feature`
3. Make changes and test thoroughly
4. Run test suite: `./test_Glide.sh`
5. Submit pull request

### Code Style
- Follow existing C++ style conventions
- Use meaningful variable names
- Add comments for complex logic
- Include error handling for all operations

### Testing
- Test on both Windows and Linux
- Verify encryption functionality
- Test edge cases and error conditions
- Performance testing with high input frequency

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Support

For issues, questions, or contributions:
- **GitHub Issues**: [Report bugs or request features](https://github.com/kareem2099/Glide/issues)
- **Documentation**: Check this README and code comments
- **Community**: Join discussions in GitHub Discussions

## Changelog

### Version 2.0.0 (Current)
- ✅ Complete rewrite with proper bidirectional communication
- ✅ End-to-end encryption with Diffie-Hellman key exchange
- ✅ Fixed screen transition logic and cursor management
- ✅ Standardized input message format
- ✅ Comprehensive error handling and logging
- ✅ Cross-platform compatibility improvements
- ✅ Performance optimizations for smooth transitions

### Version 1.0.0 (Previous)
- Basic Glide functionality
- Simple UDP communication
- Windows and Linux support
- File transfer capabilities

---

**Glide Glide** - Seamless mouse and keyboard sharing between devices with enterprise-grade security and reliability.
