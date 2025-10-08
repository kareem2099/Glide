#ifndef CLIENT_INPUT_INJECTION_H
#define CLIENT_INPUT_INJECTION_H

#include <string>
#include <netinet/in.h> // For sockaddr_in

#ifdef _WIN32
    #include <winsock2.h>
    #include <windows.h>
#endif

class EncryptionManager; // Forward declaration

#ifdef _WIN32
// Function to inject input events on Windows
void inject_input_windows(const std::string& message, EncryptionManager& encryptionManager, bool& isEncrypted);
#else
// Function to inject input events on Linux
void inject_input_linux(const std::string& message, EncryptionManager& encryptionManager, bool& isEncrypted);
#endif

#endif // CLIENT_INPUT_INJECTION_H
