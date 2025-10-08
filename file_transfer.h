#ifndef FILE_TRANSFER_H
#define FILE_TRANSFER_H

#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <functional>
#include <memory>
#include <vector>
#include <chrono>
#include <atomic>
#include <mutex>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

#include "constants.h"

// Forward declarations
struct FileTransferProgress {
    std::string filename;
    size_t fileSize;
    size_t bytesTransferred;
    size_t bytesPerSecond;
    int percentage;
    bool isComplete;
    std::string status;
};

struct FileTransferMetadata {
    std::string filename;
    size_t fileSize;
    std::string checksum;
    bool compressed;
    size_t compressedSize;
    std::string lastModified;
};

class FileTransferManager {
public:
    // Progress callback type
    using ProgressCallback = std::function<void(const FileTransferProgress&)>;

    // Constructor
    FileTransferManager();

    // Destructor
    ~FileTransferManager();

    // Enhanced file sending with progress tracking
    bool sendFile(const std::string& serverIp,
                  const std::string& filePath,
                  ProgressCallback progressCallback = nullptr,
                  bool enableCompression = true,
                  bool enableResume = true);

    // Enhanced file receiving
    bool receiveFile(int clientSocket,
                     const std::string& outputDirectory = "",
                     ProgressCallback progressCallback = nullptr);

    // Cancel current transfer
    void cancelTransfer();

    // Check if transfer is cancelled
    bool isCancelled() const;

    // Queue management methods
    std::string generateJobId();
    bool addTransferToQueue(const std::string& serverIp,
                           const std::string& filePath,
                           const std::string& outputDirectory = "",
                           ProgressCallback progressCallback = nullptr,
                           bool enableCompression = true,
                           bool enableResume = true,
                           int priority = 0,
                           bool isUpload = true);
    bool processQueue();
    void clearQueue();
    size_t getQueueSize();
    std::string getCurrentJobId() const;
    bool isQueueProcessing() const;

    // State persistence methods
    bool saveTransferState(const std::string& sessionId = "");
    bool loadTransferState(const std::string& sessionId);
    bool resumeTransfer(const std::string& sessionId,
                       ProgressCallback progressCallback = nullptr);
    std::vector<std::string> getSavedSessions();
    bool deleteTransferState(const std::string& sessionId);
    void setStateFilePath(const std::string& filePath);

    // Calculate file checksum
    static std::string calculateFileChecksum(const std::string& filePath);

    // Compress file data
    static std::vector<uint8_t> compressFileData(const std::vector<uint8_t>& data);

    // Decompress file data
    static std::vector<uint8_t> decompressFileData(const std::vector<uint8_t>& compressedData);

private:
    // Internal transfer methods
    bool sendFileWithResume(const std::string& serverIp,
                           const std::string& filePath,
                           ProgressCallback progressCallback,
                           bool enableCompression);

    bool sendFileChunked(const std::string& serverIp,
                        const std::string& filePath,
                        ProgressCallback progressCallback,
                        bool enableCompression);

    bool receiveFileWithVerification(int clientSocket,
                                    const std::string& outputDirectory,
                                    ProgressCallback progressCallback);

    // Utility methods
    std::string getFileLastModified(const std::string& filePath);
    bool validateFileTransfer(const std::string& filePath, const std::string& expectedChecksum);

public:
    // Public utility methods for derived classes
    size_t getFileSize(const std::string& filePath);

    // Network utilities
    bool sendData(int socket, const void* data, size_t size);
    bool receiveData(int socket, void* buffer, size_t size, size_t* bytesReceived = nullptr);
    bool sendString(int socket, const std::string& str);
    bool receiveString(int socket, std::string& str, size_t maxLength = Constants::FileTransfer::MAX_FILENAME_LENGTH);

    // Progress tracking
    std::chrono::steady_clock::time_point m_lastProgressUpdate;
    size_t m_lastBytesTransferred;

    // Cancellation support
    std::atomic<bool> m_cancelled;

    // Transfer queue management
    struct TransferJob {
        std::string serverIp;
        std::string filePath;
        std::string outputDirectory;
        ProgressCallback progressCallback;
        bool enableCompression;
        bool enableResume;
        int priority; // Higher number = higher priority
        std::string jobId;
        std::chrono::steady_clock::time_point createdTime;
        bool isUpload; // true for upload, false for download

        TransferJob() : priority(0), isUpload(true), createdTime(std::chrono::steady_clock::now()) {}
    };

    std::vector<TransferJob> m_transferQueue;
    std::mutex m_queueMutex;
    std::atomic<bool> m_queueProcessing;
    std::string m_currentJobId;

    // Transfer state persistence
    struct TransferState {
        std::string sessionId;
        std::string filePath;
        std::string serverIp;
        size_t fileSize;
        size_t bytesTransferred;
        std::string checksum;
        bool isUpload;
        bool enableCompression;
        std::chrono::steady_clock::time_point startTime;
        std::string status;

        TransferState() : fileSize(0), bytesTransferred(0), isUpload(true), startTime(std::chrono::steady_clock::now()) {}
    };

    TransferState m_currentTransferState;
    std::string m_transferStateFile;
    std::mutex m_stateMutex;
};

// Legacy functions for backward compatibility
#ifdef _WIN32
void send_file_tcp(const std::string& serverIp, const std::string& filePath);
#else
void send_file_tcp_linux(const std::string& serverIp, const std::string& filePath);
#endif // _WIN32

// Usage Examples:
//
// 1. Basic file transfer with progress tracking:
//    FileTransferManager transferManager;
//    auto progressCallback = [](const FileTransferProgress& progress) {
//        std::cout << "Progress: " << progress.percentage << "% ("
//                  << progress.bytesTransferred << "/" << progress.fileSize << " bytes)"
//                  << " - " << progress.bytesPerSecond << " bytes/sec" << std::endl;
//    };
//    transferManager.sendFile("192.168.1.100", "/path/to/file.txt", progressCallback);
//
// 2. Enhanced file transfer with compression:
//    transferManager.sendFile("192.168.1.100", "/path/to/large_file.zip",
//                           progressCallback, true, false);
//
// 3. File receiving with progress tracking:
//    FileTransferManager transferManager;
//    transferManager.receiveFile(clientSocket, "/output/directory", progressCallback);
//
// 4. Calculate file checksum:
//    std::string checksum = FileTransferManager::calculateFileChecksum("/path/to/file");

#endif // FILE_TRANSFER_H
