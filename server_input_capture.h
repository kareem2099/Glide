#define SERVER_INPUT_CAPTURE_H

#include <string>
#include <netinet/in.h> // For sockaddr_in

#ifdef _WIN32
    #include <winsock2.h>
    #include <windows.h>
#endif

class EncryptionManager; // Forward declaration

#ifdef _WIN32
// Function to start capturing input events on Windows
void capture_input_windows(SOCKET udpSocket, const sockaddr_in& clientAddr, EncryptionManager& encryptionManager, bool& isEncrypted);
#else
// Function to start capturing input events on Linux
void capture_input_linux(int udp_sock, const sockaddr_in& client_udp_addr, EncryptionManager& encryptionManager, bool& isEncrypted);
#endif // SERVER_INPUT_CAPTURE_H
