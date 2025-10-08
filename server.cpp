#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <cstring> // For memset
#include <fstream> // For file operations
#include <atomic>
#include <chrono>
#include "constants.h"
#include "file_transfer_server.h"
#include "encryption_manager.h" // For encryption
#include "server_input_capture.h" // For input capture functions
#include "utils.h" // For utility functions

// Global encryption manager instance for server
EncryptionManager g_encryptionManager;
// Flag to indicate if encryption is active
bool g_isEncrypted = false;
// Flag to indicate if client is connected
bool g_clientConnected = false;
// Client address for communication
struct sockaddr_in g_clientAddr;

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h> // For inet_pton
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h> // For inet_pton
    #include <unistd.h>
#endif

// Function to handle incoming messages from client
void handleClientMessages(int socket) {
    char buffer[Constants::Network::BUFFER_SIZE];
    struct sockaddr_in fromAddr;
    socklen_t fromLen = sizeof(fromAddr);
    
    while (true) {
        memset(buffer, 0, Constants::Network::BUFFER_SIZE);
        ssize_t len = recvfrom(socket, buffer, sizeof(buffer) - 1, 0, 
                              (struct sockaddr*)&fromAddr, &fromLen);
        
        if (len > 0) {
            buffer[len] = '\0';
            std::string received_message(buffer);
            
            Utils::logDebug("Server received: " + received_message);
            
            // Handle key exchange messages
            if (Utils::startsWith(received_message, Constants::EncryptionMessages::DH_PUBLIC_KEY)) {
                std::string public_key_pem = Utils::extractMessageData(received_message);
                if (g_encryptionManager.setDhPeerPublicKeyPem(public_key_pem)) {
                    if (g_encryptionManager.computeSharedSecret()) {
                        g_isEncrypted = true;
                        g_clientConnected = true;
                        g_clientAddr = fromAddr;
                        
                        Utils::logMessage(Constants::LogTypes::SUCCESS, 
                                        "Key exchange completed, encryption active");
                        
                        // Send confirmation
                        std::string confirm_msg = Constants::EncryptionMessages::KEY_EXCHANGE_COMPLETE + "OK";
                        Utils::sendSecureMessage(socket, fromAddr, confirm_msg, 
                                               g_encryptionManager, false);
                    }
                }
            }
            // Handle heartbeat messages
            else if (Utils::startsWith(received_message, Constants::ProtocolMessages::HEARTBEAT)) {
                std::string ack_msg = Constants::ProtocolMessages::HEARTBEAT_ACK + "OK";
                Utils::sendSecureMessage(socket, fromAddr, ack_msg, g_encryptionManager, false);
            }
            // Handle encrypted messages
            else if (Utils::startsWith(received_message, Constants::EncryptionMessages::ENCRYPTED_DATA)) {
                if (g_isEncrypted) {
                    std::string encrypted_data = Utils::extractMessageData(received_message);
                    std::vector<unsigned char> decoded_data = Utils::base64_decode(encrypted_data);
                    std::string decrypted_message = g_encryptionManager.decrypt(decoded_data);
                    
                    if (!decrypted_message.empty()) {
                        Utils::logDebug("Decrypted message: " + decrypted_message);
                        // Process the decrypted message as needed
                    }
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int main(int argc, char const *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <Client IP Address>" << std::endl;
        return 1;
    }

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }

    // Setup UDP socket for input
    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == INVALID_SOCKET) {
        std::cerr << "UDP socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in clientAddr;
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(Constants::Network::UDP_PORT);
    if (inet_pton(AF_INET, argv[1], &clientAddr.sin_addr) != 1) {
        std::cerr << "Invalid client IP address: " << argv[1] << std::endl;
        closesocket(udpSocket);
        WSACleanup();
        return 1;
    }

    g_clientAddr = clientAddr;

    Utils::logMessage(Constants::LogTypes::INFO, 
                     "Server started. Target client: " + std::string(argv[1]) + 
                     ":" + std::to_string(Constants::Network::UDP_PORT));

    // Initiate Diffie-Hellman key exchange
    if (!g_encryptionManager.generateDhParams()) {
        Utils::logError("main", "Failed to generate DH parameters");
        closesocket(udpSocket);
        WSACleanup();
        return 1;
    }

    std::string dh_params_pem = g_encryptionManager.getDhParamsPem();
    std::string dh_params_message = Constants::EncryptionMessages::DH_PARAMS + dh_params_pem;
    
    if (!Utils::sendSecureMessage(udpSocket, clientAddr, dh_params_message, 
                                 g_encryptionManager, false)) {
        Utils::logError("main", "Failed to send DH parameters");
        closesocket(udpSocket);
        WSACleanup();
        return 1;
    }

    Utils::logMessage(Constants::LogTypes::INFO, "Sent DH parameters to client");

    // Start message handling thread
    std::thread message_thread(handleClientMessages, udpSocket);
    message_thread.detach();

    // Start TCP file transfer server in a separate thread
    std::thread file_transfer_thread(handle_file_transfer_server);
    file_transfer_thread.detach();

    // Wait for key exchange to complete
    int timeout = 0;
    while (!g_isEncrypted && timeout < 30) { // 30 second timeout
        std::this_thread::sleep_for(std::chrono::seconds(1));
        timeout++;
    }

    if (!g_isEncrypted) {
        Utils::logError("main", "Key exchange timeout");
        closesocket(udpSocket);
        WSACleanup();
        return 1;
    }

    // Start input capture
    capture_input_windows(udpSocket, clientAddr, g_encryptionManager, g_isEncrypted);

    closesocket(udpSocket);
    WSACleanup();

#else // Linux
    // Setup UDP socket for input
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        Utils::logError("main", "UDP socket creation failed");
        return 1;
    }

    struct sockaddr_in client_udp_addr{};
    client_udp_addr.sin_family = AF_INET;
    client_udp_addr.sin_port = htons(Constants::Network::UDP_PORT);
    if (inet_pton(AF_INET, argv[1], &client_udp_addr.sin_addr) != 1) {
        Utils::logError("main", "Invalid client IP address: " + std::string(argv[1]));
        close(udp_sock);
        return 1;
    }

    g_clientAddr = client_udp_addr;

    Utils::logMessage(Constants::LogTypes::INFO, 
                     "Server started. Target client: " + std::string(argv[1]) + 
                     ":" + std::to_string(Constants::Network::UDP_PORT));

    // Initiate Diffie-Hellman key exchange
    if (!g_encryptionManager.generateDhParams()) {
        Utils::logError("main", "Failed to generate DH parameters");
        close(udp_sock);
        return 1;
    }

    std::string dh_params_pem = g_encryptionManager.getDhParamsPem();
    std::string dh_params_message = Constants::EncryptionMessages::DH_PARAMS + dh_params_pem;
    
    if (!Utils::sendSecureMessage(udp_sock, client_udp_addr, dh_params_message, 
                                 g_encryptionManager, false)) {
        Utils::logError("main", "Failed to send DH parameters");
        close(udp_sock);
        return 1;
    }

    Utils::logMessage(Constants::LogTypes::INFO, "Sent DH parameters to client");

    // Start message handling thread
    std::thread message_thread(handleClientMessages, udp_sock);
    message_thread.detach();

    // Start TCP file transfer server in a separate thread
    std::thread file_transfer_thread(FileTransferServer::handle_file_transfer_server_linux);
    file_transfer_thread.detach();

    // Wait for key exchange to complete
    int timeout = 0;
    while (!g_isEncrypted && timeout < 30) { // 30 second timeout
        std::this_thread::sleep_for(std::chrono::seconds(1));
        timeout++;
    }

    if (!g_isEncrypted) {
        Utils::logError("main", "Key exchange timeout");
        close(udp_sock);
        return 1;
    }

    // Start input capture
    capture_input_linux(udp_sock, client_udp_addr, g_encryptionManager, g_isEncrypted);

    close(udp_sock);
#endif

    return 0;
}
