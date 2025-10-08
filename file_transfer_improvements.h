#ifndef FILE_TRANSFER_IMPROVEMENTS_H
#define FILE_TRANSFER_IMPROVEMENTS_H

#include "file_transfer.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <future>
#include <unordered_map>

// Enhanced transfer modes
enum class TransferMode {
    TCP_STANDARD,      // Current TCP implementation
    TCP_PARALLEL,      // Multi-threaded TCP
    UDP_ACCELERATED,   // UDP for small files
    HYBRID_ADAPTIVE    // Auto-select best method
};

// Network condition detection
struct NetworkConditions {
    double latency_ms = 0.0;
    double bandwidth_mbps = 0.0;
    int packet_loss_percent = 0;
    bool is_congested = false;

    std::string to_string() const;
};

// Enhanced transfer statistics
struct TransferStatistics {
    size_t total_bytes_transferred = 0;
    size_t total_files_transferred = 0;
    double average_speed_mbps = 0.0;
    size_t failed_transfers = 0;
    std::chrono::seconds total_transfer_time{0};

    void update(size_t bytes, std::chrono::milliseconds duration);
    std::string to_string() const;
};

// Advanced file transfer manager with improvements
class EnhancedFileTransferManager : public FileTransferManager {
public:
    EnhancedFileTransferManager();
    ~EnhancedFileTransferManager();

    // Enhanced transfer methods
    bool sendFileAdvanced(const std::string& serverIp,
                         const std::string& filePath,
                         ProgressCallback progressCallback = nullptr,
                         TransferMode mode = TransferMode::HYBRID_ADAPTIVE,
                         bool enableEncryption = true,
                         int maxRetries = 3);

    bool receiveFileAdvanced(int clientSocket,
                            const std::string& outputDirectory = "",
                            ProgressCallback progressCallback = nullptr,
                            bool enableEncryption = true);

    // Network analysis
    NetworkConditions analyzeNetworkConditions(const std::string& targetIp);
    TransferMode selectOptimalTransferMode(const std::string& filePath,
                                          const NetworkConditions& conditions);

    // Parallel transfer support
    bool sendFileParallel(const std::string& serverIp,
                         const std::string& filePath,
                         ProgressCallback progressCallback = nullptr,
                         int numThreads = 4);

    // UDP accelerated transfer for small files
    bool sendFileUDP(const std::string& serverIp,
                    const std::string& filePath,
                    ProgressCallback progressCallback = nullptr);

    // Differential transfer (send only changes)
    bool sendFileDifferential(const std::string& serverIp,
                             const std::string& filePath,
                             const std::string& referenceFile,
                             ProgressCallback progressCallback = nullptr);

    // Transfer scheduling
    bool scheduleTransfer(const std::string& serverIp,
                         const std::string& filePath,
                         std::chrono::system_clock::time_point scheduleTime,
                         ProgressCallback progressCallback = nullptr);

    // Statistics and monitoring
    TransferStatistics getStatistics() const { return m_statistics; }
    void resetStatistics();

    // Connection pooling
    bool initializeConnectionPool(size_t poolSize = 5);
    void cleanupConnectionPool();

private:
    // Implementation methods
    bool sendFileWithMode(const std::string& serverIp,
                         const std::string& filePath,
                         ProgressCallback progressCallback,
                         TransferMode mode,
                         bool enableEncryption,
                         int maxRetries);

    bool sendFileParallelImpl(const std::string& serverIp,
                             const std::string& filePath,
                             ProgressCallback progressCallback,
                             int numThreads);

    bool sendFileUDPImpl(const std::string& serverIp,
                        const std::string& filePath,
                        ProgressCallback progressCallback);

    // Network utilities
    double measureLatency(const std::string& targetIp);
    double measureBandwidth(const std::string& targetIp);
    int measurePacketLoss(const std::string& targetIp);

    // Encryption support
    bool initializeEncryption();
    std::vector<uint8_t> encryptFileData(const std::vector<uint8_t>& data);
    std::vector<uint8_t> decryptFileData(const std::vector<uint8_t>& encryptedData);

    // Thread management
    std::vector<std::thread> m_workerThreads;
    std::atomic<bool> m_shutdown{false};
    std::mutex m_threadMutex;
    std::condition_variable m_threadCV;

    // Connection pool
    struct ConnectionInfo {
        int socket_fd;
        std::string target_ip;
        std::chrono::steady_clock::time_point last_used;
        bool in_use;
    };

    std::vector<ConnectionInfo> m_connectionPool;
    std::mutex m_poolMutex;
    std::condition_variable m_poolCV;

    // Statistics
    TransferStatistics m_statistics;
    std::mutex m_statsMutex;

    // Encryption keys
    std::vector<uint8_t> m_aesKey;
    std::vector<uint8_t> m_aesIV;
    bool m_encryptionInitialized{false};
};

// Utility functions for file operations
namespace FileTransferUtils {
    // Calculate file hash for differential transfer
    std::string calculateFileHash(const std::string& filePath);

    // Generate delta between two files
    std::vector<uint8_t> generateFileDelta(const std::string& originalFile,
                                          const std::string& modifiedFile);

    // Apply delta to file
    bool applyFileDelta(const std::string& originalFile,
                       const std::string& outputFile,
                       const std::vector<uint8_t>& delta);

    // Compress data with multiple algorithms
    std::vector<uint8_t> compressDataAdvanced(const std::vector<uint8_t>& data,
                                             int compressionLevel = 6);

    // Adaptive buffer sizing based on file size and network conditions
    size_t calculateOptimalBufferSize(size_t fileSize,
                                     const NetworkConditions& conditions);
}

#endif // FILE_TRANSFER_IMPROVEMENTS_H
