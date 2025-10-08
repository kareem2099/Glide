#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

// Forward declarations
class EncryptionManager;
struct sockaddr_in;

namespace Utils {
    // Base64 encoding/decoding functions
    std::string base64_encode(const std::vector<unsigned char>& data);
    std::vector<unsigned char> base64_decode(const std::string& encoded_string);
    
    // Message validation and parsing
    bool isValidMessage(const std::string& message);
    std::string extractMessageType(const std::string& message);
    std::string extractMessageData(const std::string& message);
    
    // Network utilities
    std::string formatMessage(const std::string& type, const std::string& data);
    bool sendSecureMessage(int socket, const struct sockaddr_in& addr, 
                          const std::string& message, EncryptionManager& encMgr, bool encrypted);
    
    // Logging utilities
    void logMessage(const std::string& level, const std::string& message);
    void logError(const std::string& function, const std::string& error);
    void logDebug(const std::string& message);
    
    // Coordinate system utilities
    struct Point {
        int x, y;
        Point(int x = 0, int y = 0) : x(x), y(y) {}
    };
    
    Point translateCoordinates(const Point& source, int sourceWidth, int sourceHeight,
                              int targetWidth, int targetHeight);
    
    // Input validation
    bool isValidKeyCode(int keyCode);
    bool isValidMouseButton(int button);
    bool isValidCoordinate(int x, int y, int screenWidth, int screenHeight);
    
    // String utilities
    std::vector<std::string> split(const std::string& str, char delimiter);
    std::string trim(const std::string& str);
    bool startsWith(const std::string& str, const std::string& prefix);
    bool endsWith(const std::string& str, const std::string& suffix);
}

#endif // UTILS_H
