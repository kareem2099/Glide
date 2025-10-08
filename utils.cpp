#include "utils.h"
#include "constants.h"
#include "encryption_manager.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

namespace Utils {

// Base64 encoding table
static const std::string base64_chars = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(const std::vector<unsigned char>& data) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    
    const unsigned char* bytes_to_encode = data.data();
    int in_len = data.size();

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
            ret += '=';
    }

    return ret;
}

std::vector<unsigned char> base64_decode(const std::string& encoded_string) {
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::vector<unsigned char> ret;

    while (in_len-- && (encoded_string[in] != '=') && is_base64(encoded_string[in])) {
        char_array_4[i++] = encoded_string[in]; in++;
        if (i ==4) {
            for (i = 0; i <4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret.push_back(char_array_3[i]);
            i = 0;
        }
    }

    if (i) {
        for (j = i; j <4; j++)
            char_array_4[j] = 0;

        for (j = 0; j <4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
    }

    return ret;
}

bool isValidMessage(const std::string& message) {
    if (message.empty() || message.length() > Constants::Network::MAX_MESSAGE_SIZE) {
        return false;
    }
    
    // Check if message has a valid format (TYPE:DATA)
    size_t colon_pos = message.find(':');
    if (colon_pos == std::string::npos || colon_pos == 0) {
        return false;
    }
    
    return true;
}

std::string extractMessageType(const std::string& message) {
    size_t colon_pos = message.find(':');
    if (colon_pos == std::string::npos) {
        return "";
    }
    return message.substr(0, colon_pos + 1); // Include the colon
}

std::string extractMessageData(const std::string& message) {
    size_t colon_pos = message.find(':');
    if (colon_pos == std::string::npos || colon_pos + 1 >= message.length()) {
        return "";
    }
    return message.substr(colon_pos + 1);
}

std::string formatMessage(const std::string& type, const std::string& data) {
    return type + data;
}

bool sendSecureMessage(int socket, const struct sockaddr_in& addr, 
                      const std::string& message, EncryptionManager& encMgr, bool encrypted) {
    std::string finalMessage;
    
    if (encrypted && encMgr.hasSharedSecret()) {
        // Encrypt the message
        std::vector<unsigned char> encryptedData = encMgr.encrypt(message);
        if (encryptedData.empty()) {
            logError("sendSecureMessage", "Failed to encrypt message");
            return false;
        }
        
        // Encode to base64 and format
        std::string encodedData = base64_encode(encryptedData);
        finalMessage = formatMessage(Constants::EncryptionMessages::ENCRYPTED_DATA, encodedData);
    } else {
        finalMessage = message;
    }
    
    // Send the message
    int result = sendto(socket, finalMessage.c_str(), finalMessage.length(), 0, 
                       (const struct sockaddr*)&addr, sizeof(addr));
    
    if (result < 0) {
        logError("sendSecureMessage", "Failed to send message");
        return false;
    }
    
    return true;
}

void logMessage(const std::string& level, const std::string& message) {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    
    std::cout << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] "
              << "[" << level << "] " << message << std::endl;
}

void logError(const std::string& function, const std::string& error) {
    logMessage(Constants::LogTypes::ERROR, function + ": " + error);
}

void logDebug(const std::string& message) {
    logMessage(Constants::LogTypes::DEBUG, message);
}

Point translateCoordinates(const Point& source, int sourceWidth, int sourceHeight,
                          int targetWidth, int targetHeight) {
    if (sourceWidth <= 0 || sourceHeight <= 0 || targetWidth <= 0 || targetHeight <= 0) {
        return source; // Return original if invalid dimensions
    }
    
    // Scale coordinates proportionally
    int newX = (source.x * targetWidth) / sourceWidth;
    int newY = (source.y * targetHeight) / sourceHeight;
    
    // Ensure coordinates are within bounds
    newX = std::max(0, std::min(newX, targetWidth - 1));
    newY = std::max(0, std::min(newY, targetHeight - 1));
    
    return Point(newX, newY);
}

bool isValidKeyCode(int keyCode) {
    // Basic validation for key codes
    return keyCode > 0 && keyCode < 65536; // Reasonable range for key codes
}

bool isValidMouseButton(int button) {
    return button >= 1 && button <= 5; // Left, Right, Middle, X1, X2
}

bool isValidCoordinate(int x, int y, int screenWidth, int screenHeight) {
    return x >= 0 && x < screenWidth && y >= 0 && y < screenHeight;
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

bool startsWith(const std::string& str, const std::string& prefix) {
    return str.length() >= prefix.length() && 
           str.compare(0, prefix.length(), prefix) == 0;
}

bool endsWith(const std::string& str, const std::string& suffix) {
    return str.length() >= suffix.length() && 
           str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

} // namespace Utils
