#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>

namespace Constants {
    const int DEFAULT_UDP_PORT = 45454;
    const std::string APP_NAME = "Glide App";
    const std::string ORGANIZATION_NAME = "Glide Team";
    const std::string SETTINGS_FILE_NAME = "Settings";

    namespace SettingsKeys {
        const std::string SERVER_UDP_PORT = "server/udpPort";
        const std::string CLIENT_SERVER_IP = "client/serverIP";
        const std::string CLIENT_UDP_PORT = "client/udpPort";
        const std::string WINDOW_GEOMETRY = "window/geometry";

        // General Settings
        const std::string LANGUAGE = "general/language";
        const std::string SCREEN_NAME = "general/screenName";
        const std::string MINIMIZE_TO_TRAY = "general/minimizeToTray";
        const std::string HIDE_ON_STARTUP = "general/hideOnStartup";
        const std::string START_ON_STARTUP = "general/startOnStartup";

        // Networking Settings
        const std::string NETWORK_PORT = "networking/port";
        const std::string NETWORK_ADDRESS = "networking/address";
        const std::string ENABLE_SSL = "networking/enableSsl";
        const std::string REQUIRE_CLIENT_CERT = "networking/requireClientCert";

        // Logging Settings
        const std::string LOG_LEVEL = "logging/level";
        const std::string LOG_FILE_PATH = "logging/filePath";
        const std::string DEVICE_ROLE = "general/deviceRole"; // New setting for device role

        // Glide Settings
        const std::string EDGE_SENSITIVITY = "Glide/edgeSensitivity";
        const std::string DEAD_ZONE_SIZE = "Glide/deadZoneSize";
        const std::string TRANSITION_DELAY = "Glide/transitionDelay";
        const std::string AUTO_RECONNECT = "Glide/autoReconnect";
    } // namespace SettingsKeys

    namespace LogTypes {
        const std::string INFO = "INFO";
        const std::string ERROR = "ERROR";
        const std::string SUCCESS = "SUCCESS";
        const std::string WARNING = "WARNING";
        const std::string DEBUG = "DEBUG";
    } // namespace LogTypes

    namespace Network {
        const int UDP_PORT = 45454;
        const int TCP_PORT = 12345; // For file transfer
        const int BUFFER_SIZE = 4096; // Increased for encrypted messages
        const int FILE_BUFFER_SIZE = 65536; // 64KB buffer for file transfers
        const int MAX_MESSAGE_SIZE = 8192;
        const int HEARTBEAT_INTERVAL = 5000; // milliseconds
        const int CONNECTION_TIMEOUT = 15000; // milliseconds
        const int RECONNECT_DELAY = 2000; // milliseconds
    } // namespace Network

    namespace FileTransfer {
        const int MAX_FILENAME_LENGTH = 255;
        const int MAX_CHUNK_SIZE = 1024 * 1024; // 1MB chunks for large files
        const int COMPRESSION_THRESHOLD = 1024; // Only compress files > 1KB
        const int MAX_RESUME_ATTEMPTS = 3;
        const int TRANSFER_TIMEOUT = 30000; // 30 seconds timeout
        const int PROGRESS_UPDATE_INTERVAL = 1000; // 1 second between progress updates
    } // namespace FileTransfer

    namespace InputMessages {
        const std::string SCREEN_EXIT = "SCREEN_EXIT:";
        const std::string SCREEN_ENTER = "SCREEN_ENTER:";
        const std::string KEY_PRESS = "KEY_PRESS:";
        const std::string KEY_RELEASE = "KEY_RELEASE:";
        const std::string MOUSE_MOVE = "MOUSE_MOVE:";
        const std::string MOUSE_PRESS = "MOUSE_PRESS:";
        const std::string MOUSE_RELEASE = "MOUSE_RELEASE:";
        const std::string MOUSE_SCROLL = "MOUSE_SCROLL:";
        const std::string MOUSE_SCROLL_UP = "UP";
        const std::string MOUSE_SCROLL_DOWN = "DOWN";
        
        // Enhanced input messages
        const std::string BATCH_INPUT = "BATCH_INPUT:";
        const std::string COORDINATE_SYNC = "COORD_SYNC:";
        const std::string MODIFIER_STATE = "MOD_STATE:";
    } // namespace InputMessages

    namespace ClipboardMessages {
        const std::string CLIPBOARD_DATA = "CLIPBOARD_DATA:";
        const std::string CLIPBOARD_REQUEST = "CLIPBOARD_REQUEST:";
        const std::string CLIPBOARD_SYNC = "CLIPBOARD_SYNC:";
    } // namespace ClipboardMessages

    namespace EncryptionMessages {
        const std::string DH_PARAMS = "DH_PARAMS:";
        const std::string DH_PUBLIC_KEY = "DH_PUBLIC_KEY:";
        const std::string ENCRYPTED_DATA = "ENCRYPTED_DATA:";
        const std::string KEY_EXCHANGE_COMPLETE = "KEY_EXCHANGE_COMPLETE:";
        const std::string ENCRYPTION_ERROR = "ENCRYPTION_ERROR:";
    } // namespace EncryptionMessages

    namespace ProtocolMessages {
        const std::string HANDSHAKE_REQUEST = "HANDSHAKE_REQUEST:";
        const std::string HANDSHAKE_RESPONSE = "HANDSHAKE_RESPONSE:";
        const std::string HEARTBEAT = "HEARTBEAT:";
        const std::string HEARTBEAT_ACK = "HEARTBEAT_ACK:";
        const std::string CONNECTION_ESTABLISHED = "CONNECTION_ESTABLISHED:";
        const std::string DISCONNECT = "DISCONNECT:";
        const std::string ERROR_MESSAGE = "ERROR:";
        const std::string ACK = "ACK:";
        const std::string NACK = "NACK:";
    } // namespace ProtocolMessages

    namespace ScreenTransition {
        const std::string DIRECTION_LEFT = "LEFT";
        const std::string DIRECTION_RIGHT = "RIGHT";
        const std::string DIRECTION_UP = "UP";
        const std::string DIRECTION_DOWN = "DOWN";
        const int DEFAULT_EDGE_SENSITIVITY = 5; // pixels
        const int DEFAULT_DEAD_ZONE = 10; // pixels
        const int DEFAULT_TRANSITION_DELAY = 100; // milliseconds
    } // namespace ScreenTransition

    namespace ErrorCodes {
        const int SUCCESS = 0;
        const int NETWORK_ERROR = 1;
        const int ENCRYPTION_ERROR = 2;
        const int INPUT_ERROR = 3;
        const int PROTOCOL_ERROR = 4;
        const int TIMEOUT_ERROR = 5;
        const int INVALID_MESSAGE = 6;
    } // namespace ErrorCodes
} // namespace Constants

#endif // CONSTANTS_H
