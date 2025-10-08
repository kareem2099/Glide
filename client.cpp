#include <iostream>
#include <string>
#include <thread>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <chrono>
#include "constants.h"
#include "encryption_manager.h" // For encryption
#include "client_input_injection.h" // For input injection functions
#include "utils.h" // For utility functions

// Global encryption manager instance for client
EncryptionManager g_encryptionManager;
// Flag to indicate if encryption is active
bool g_isEncrypted = false;
// Flag to indicate if server is connected
bool g_serverConnected = false;
// Server address for communication
struct sockaddr_in g_serverAddr;

#ifdef _WIN32
    #include <winsock2.h>
    #include <windows.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <X11/extensions/XTest.h>
    #include <X11/cursorfont.h>
#endif


int main(int argc, char const *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <Server IP Address>" << std::endl;
        return 1;
    }

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        Utils::logError("main", "WSAStartup failed");
        return 1;
    }

    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == INVALID_SOCKET) {
        Utils::logError("main", "UDP socket creation failed");
        WSACleanup();
        return 1;
    }

    // Setup server address for sending responses
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(Constants::Network::UDP_PORT);
    if (inet_pton(AF_INET, argv[1], &serverAddr.sin_addr) != 1) {
        Utils::logError("main", "Invalid server IP address: " + std::string(argv[1]));
        closesocket(udpSocket);
        WSACleanup();
        return 1;
    }
    g_serverAddr = serverAddr;

    // Bind to local port for receiving
    sockaddr_in localAddr{};
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(Constants::Network::UDP_PORT);
    localAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udpSocket, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
        Utils::logError("main", "Bind failed");
        closesocket(udpSocket);
        WSACleanup();
        return 1;
    }

    Utils::logMessage(Constants::LogTypes::INFO, 
                     "Client listening on port " + std::to_string(Constants::Network::UDP_PORT));

    char buffer[Constants::Network::BUFFER_SIZE];
    struct sockaddr_in fromAddr;
    int fromLen = sizeof(fromAddr);

    while (true) {
        memset(buffer, 0, Constants::Network::BUFFER_SIZE);
        int len = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0, 
                          (sockaddr*)&fromAddr, &fromLen);
        
        if (len > 0) {
            buffer[len] = '\0';
            std::string received_message(buffer);
            
            Utils::logDebug("Client received: " + received_message);

            // Handle key exchange messages
            if (Utils::startsWith(received_message, Constants::EncryptionMessages::DH_PARAMS)) {
                std::string dh_params_pem = Utils::extractMessageData(received_message);
                
                if (g_encryptionManager.setDhParamsPem(dh_params_pem)) {
                    if (g_encryptionManager.generateDhKeyPair()) {
                        // Send our public key back to server
                        std::string public_key_pem = g_encryptionManager.getDhPublicKeyPem();
                        std::string response_message = Constants::EncryptionMessages::DH_PUBLIC_KEY + public_key_pem;
                        
                        if (Utils::sendSecureMessage(udpSocket, fromAddr, response_message, 
                                                   g_encryptionManager, false)) {
                            Utils::logMessage(Constants::LogTypes::INFO, "Sent public key to server");
                            g_serverAddr = fromAddr; // Update server address
                        }
                    }
                }
            }
            // Handle key exchange completion
            else if (Utils::startsWith(received_message, Constants::EncryptionMessages::KEY_EXCHANGE_COMPLETE)) {
                g_isEncrypted = true;
                g_serverConnected = true;
                Utils::logMessage(Constants::LogTypes::SUCCESS, "Key exchange completed, encryption active");
            }
            // Handle encrypted input messages
            else if (Utils::startsWith(received_message, Constants::EncryptionMessages::ENCRYPTED_DATA)) {
                if (g_isEncrypted) {
                    std::string encrypted_data = Utils::extractMessageData(received_message);
                    std::vector<unsigned char> decoded_data = Utils::base64_decode(encrypted_data);
                    std::string decrypted_message = g_encryptionManager.decrypt(decoded_data);
                    
                    if (!decrypted_message.empty()) {
                        inject_input_windows(decrypted_message, g_encryptionManager, g_isEncrypted);
                    }
                }
            }
            // Handle unencrypted input messages (during initial setup)
            else if (!g_isEncrypted) {
                inject_input_windows(received_message, g_encryptionManager, g_isEncrypted);
            }
        } else if (len == 0) {
            Utils::logMessage(Constants::LogTypes::WARNING, "Server disconnected");
            break;
        } else {
            Utils::logError("main", "recvfrom failed");
            break;
        }
    }

    closesocket(udpSocket);
    WSACleanup();

#else // Linux
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        Utils::logError("main", "UDP socket creation failed");
        return 1;
    }

    // Setup server address for sending responses
    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(Constants::Network::UDP_PORT);
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) != 1) {
        Utils::logError("main", "Invalid server IP address: " + std::string(argv[1]));
        close(udp_sock);
        return 1;
    }
    g_serverAddr = server_addr;

    // Bind to local port for receiving
    struct sockaddr_in local_udp_addr{};
    local_udp_addr.sin_family = AF_INET;
    local_udp_addr.sin_port = htons(Constants::Network::UDP_PORT);
    local_udp_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udp_sock, (struct sockaddr *)&local_udp_addr, sizeof(local_udp_addr)) < 0) {
        Utils::logError("main", "UDP bind failed");
        close(udp_sock);
        return 1;
    }

    Utils::logMessage(Constants::LogTypes::INFO, 
                     "Client listening on port " + std::to_string(Constants::Network::UDP_PORT));

    char udp_buffer[Constants::Network::BUFFER_SIZE] = {0};
    struct sockaddr_in fromAddr;
    socklen_t fromLen = sizeof(fromAddr);

    while (true) {
        memset(udp_buffer, 0, Constants::Network::BUFFER_SIZE);
        ssize_t valread = recvfrom(udp_sock, udp_buffer, Constants::Network::BUFFER_SIZE, 0, 
                                  (struct sockaddr*)&fromAddr, &fromLen);
        
        if (valread <= 0) {
            Utils::logMessage(Constants::LogTypes::WARNING, "Server disconnected or error");
            break;
        }
        
        std::string received_message(udp_buffer);
        Utils::logDebug("Client received: " + received_message);

        // Handle key exchange messages
        if (Utils::startsWith(received_message, Constants::EncryptionMessages::DH_PARAMS)) {
            std::string dh_params_pem = Utils::extractMessageData(received_message);
            
            if (g_encryptionManager.setDhParamsPem(dh_params_pem)) {
                if (g_encryptionManager.generateDhKeyPair()) {
                    // Send our public key back to server
                    std::string public_key_pem = g_encryptionManager.getDhPublicKeyPem();
                    std::string response_message = Constants::EncryptionMessages::DH_PUBLIC_KEY + public_key_pem;
                    
                    if (Utils::sendSecureMessage(udp_sock, fromAddr, response_message, 
                                               g_encryptionManager, false)) {
                        Utils::logMessage(Constants::LogTypes::INFO, "Sent public key to server");
                        g_serverAddr = fromAddr; // Update server address
                    }
                }
            }
        }
        // Handle key exchange completion
        else if (Utils::startsWith(received_message, Constants::EncryptionMessages::KEY_EXCHANGE_COMPLETE)) {
            g_isEncrypted = true;
            g_serverConnected = true;
            Utils::logMessage(Constants::LogTypes::SUCCESS, "Key exchange completed, encryption active");
        }
        // Handle encrypted input messages
        else if (Utils::startsWith(received_message, Constants::EncryptionMessages::ENCRYPTED_DATA)) {
            if (g_isEncrypted) {
                std::string encrypted_data = Utils::extractMessageData(received_message);
                std::vector<unsigned char> decoded_data = Utils::base64_decode(encrypted_data);
                std::string decrypted_message = g_encryptionManager.decrypt(decoded_data);
                
                if (!decrypted_message.empty()) {
                    inject_input_linux(decrypted_message, g_encryptionManager, g_isEncrypted);
                }
            }
        }
        // Handle unencrypted input messages (during initial setup)
        else if (!g_isEncrypted) {
            inject_input_linux(received_message, g_encryptionManager, g_isEncrypted);
        }
    }

    close(udp_sock);
#endif

    return 0;
}
