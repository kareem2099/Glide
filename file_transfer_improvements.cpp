#include "file_transfer_improvements.h"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#endif

// Network conditions implementation
std::string NetworkConditions::to_string() const {
    std::stringstream ss;
    ss << "Latency: " << latency_ms << "ms, "
       << "Bandwidth: " << bandwidth_mbps << " Mbps, "
       << "Packet Loss: " << packet_loss_percent << "%, "
       << "Congested: " << (is_congested ? "Yes" : "No");
    return ss.str();
}

// Transfer statistics implementation
void TransferStatistics::update(size_t bytes, std::chrono::milliseconds duration) {
    total_bytes_transferred += bytes;
    total_transfer_time += std::chrono::duration_cast<std::chrono::seconds>(duration);

    if (total_transfer_time.count() > 0) {
        average_speed_mbps = (total_bytes_transferred * 8.0) /
                           (total_transfer_time.count() * 1000000.0);
    }
}

std::string TransferStatistics::to_string() const {
    std::stringstream ss;
    ss << "Total Bytes: " << total_bytes_transferred << ", "
       << "Total Files: " << total_files_transferred << ", "
       << "Average Speed: " << std::fixed << std::setprecision(2)
       << average_speed_mbps << " Mbps, "
       << "Failed: " << failed_transfers << ", "
       << "Total Time: " << total_transfer_time.count() << "s";
    return ss.str();
}

// Enhanced File Transfer Manager implementation
EnhancedFileTransferManager::EnhancedFileTransferManager()
    : FileTransferManager() {
    initializeEncryption();
    initializeConnectionPool();
}





EnhancedFileTransferManager::~EnhancedFileTransferManager() {
    m_shutdown.store(true);
    m_threadCV.notify_all();

    {
        std::lock_guard<std::mutex> lock(m_threadMutex);
        for (auto& thread : m_workerThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    cleanupConnectionPool();
}

bool EnhancedFileTransferManager::sendFileAdvanced(const std::string& serverIp,
                                                  const std::string& filePath,
                                                  ProgressCallback progressCallback,
                                                  TransferMode mode,
                                                  bool enableEncryption,
                                                  int maxRetries) {
    // Analyze network conditions
    NetworkConditions conditions = analyzeNetworkConditions(serverIp);

    // Select optimal transfer mode if auto-selected
    if (mode == TransferMode::HYBRID_ADAPTIVE) {
        mode = selectOptimalTransferMode(filePath, conditions);
    }

    // Attempt transfer with retries
    for (int attempt = 1; attempt <= maxRetries; ++attempt) {
        try {
            bool success = sendFileWithMode(serverIp, filePath, progressCallback,
                                          mode, enableEncryption, attempt);
            if (success) {
                {
                    std::lock_guard<std::mutex> lock(m_statsMutex);
                    m_statistics.total_files_transferred++;
                }
                return true;
            }
        } catch (const std::exception& e) {
            std::cerr << "Transfer attempt " << attempt << " failed: " << e.what() << std::endl;

            if (attempt < maxRetries) {
                // Exponential backoff
                std::this_thread::sleep_for(std::chrono::milliseconds(1000 * attempt));
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        m_statistics.failed_transfers++;
    }

    return false;
}

NetworkConditions EnhancedFileTransferManager::analyzeNetworkConditions(const std::string& targetIp) {
    NetworkConditions conditions;

    // Measure latency
    conditions.latency_ms = measureLatency(targetIp);

    // Measure bandwidth
    conditions.bandwidth_mbps = measureBandwidth(targetIp);

    // Measure packet loss
    conditions.packet_loss_percent = measurePacketLoss(targetIp);

    // Determine congestion
    conditions.is_congested = conditions.latency_ms > 100.0 ||
                            conditions.packet_loss_percent > 5;

    return conditions;
}

TransferMode EnhancedFileTransferManager::selectOptimalTransferMode(
    const std::string& filePath, const NetworkConditions& conditions) {

    size_t fileSize = getFileSize(filePath);

    // For small files (< 1MB), use UDP if network is good
    if (fileSize < 1024 * 1024) {
        if (conditions.latency_ms < 50.0 && conditions.packet_loss_percent < 2) {
            return TransferMode::UDP_ACCELERATED;
        }
    }

    // For large files, use parallel TCP if bandwidth is high
    if (conditions.bandwidth_mbps > 10.0 && !conditions.is_congested) {
        return TransferMode::TCP_PARALLEL;
    }

    // Default to standard TCP
    return TransferMode::TCP_STANDARD;
}

bool EnhancedFileTransferManager::sendFileParallel(const std::string& serverIp,
                                                  const std::string& filePath,
                                                  ProgressCallback progressCallback,
                                                  int numThreads) {
    return sendFileParallelImpl(serverIp, filePath, progressCallback, numThreads);
}

bool EnhancedFileTransferManager::sendFileUDP(const std::string& serverIp,
                                             const std::string& filePath,
                                             ProgressCallback progressCallback) {
    return sendFileUDPImpl(serverIp, filePath, progressCallback);
}

// Implementation methods
bool EnhancedFileTransferManager::sendFileWithMode(const std::string& serverIp,
                                                  const std::string& filePath,
                                                  ProgressCallback progressCallback,
                                                  TransferMode mode,
                                                  bool enableEncryption,
                                                  int maxRetries) {
    // For now, delegate to standard TCP implementation
    // In a full implementation, this would dispatch to different transfer methods
    return sendFile(serverIp, filePath, progressCallback, true, true);
}

bool EnhancedFileTransferManager::sendFileParallelImpl(const std::string& serverIp,
                                                      const std::string& filePath,
                                                      ProgressCallback progressCallback,
                                                      int numThreads) {
    // Simplified parallel implementation - just use standard TCP for now
    // In a full implementation, this would split the file and transfer chunks in parallel
    std::cout << "Parallel transfer using " << numThreads << " threads (simplified)" << std::endl;
    return sendFile(serverIp, filePath, progressCallback, true, true);
}

bool EnhancedFileTransferManager::sendFileUDPImpl(const std::string& serverIp,
                                                 const std::string& filePath,
                                                 ProgressCallback progressCallback) {
    // Simplified UDP implementation - just use standard TCP for now
    // In a full implementation, this would use UDP sockets for small files
    std::cout << "UDP transfer (simplified)" << std::endl;
    return sendFile(serverIp, filePath, progressCallback, true, true);
}

bool EnhancedFileTransferManager::receiveFileAdvanced(int clientSocket,
                                                     const std::string& outputDirectory,
                                                     ProgressCallback progressCallback,
                                                     bool enableEncryption) {
    // For now, delegate to standard receive implementation
    // In a full implementation, this would support enhanced features
    return receiveFile(clientSocket, outputDirectory, progressCallback);
}

bool EnhancedFileTransferManager::sendFileDifferential(const std::string& serverIp,
                                                      const std::string& filePath,
                                                      const std::string& referenceFile,
                                                      ProgressCallback progressCallback) {
    // Simplified differential transfer - just use standard TCP for now
    // In a full implementation, this would calculate and send only the differences
    std::cout << "Differential transfer (simplified)" << std::endl;
    return sendFile(serverIp, filePath, progressCallback, true, true);
}

bool EnhancedFileTransferManager::scheduleTransfer(const std::string& serverIp,
                                                  const std::string& filePath,
                                                  std::chrono::system_clock::time_point scheduleTime,
                                                  ProgressCallback progressCallback) {
    // Simplified scheduled transfer - just execute immediately for now
    // In a full implementation, this would schedule the transfer for the specified time
    std::cout << "Scheduled transfer (simplified - executing immediately)" << std::endl;
    return sendFile(serverIp, filePath, progressCallback, true, true);
}



double EnhancedFileTransferManager::measureLatency(const std::string& targetIp) {
    auto start = std::chrono::high_resolution_clock::now();

    // Simple TCP connection test
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return 1000.0; // Default high latency on error

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(Constants::Network::TCP_PORT);

    if (inet_pton(AF_INET, targetIp.c_str(), &server_addr.sin_addr) <= 0) {
        close(sock);
        return 1000.0;
    }

    auto connect_start = std::chrono::high_resolution_clock::now();
    int result = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    auto connect_end = std::chrono::high_resolution_clock::now();

    close(sock);

    if (result < 0) return 1000.0;

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        connect_end - connect_start);

    return duration.count() / 1000.0; // Convert to milliseconds
}

double EnhancedFileTransferManager::measureBandwidth(const std::string& targetIp) {
    // Simplified bandwidth test - send a test packet and measure time
    const size_t test_size = 64 * 1024; // 64KB test
    std::vector<uint8_t> test_data(test_size, 'A');

    auto start = std::chrono::high_resolution_clock::now();

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return 1.0;

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(Constants::Network::TCP_PORT);

    if (inet_pton(AF_INET, targetIp.c_str(), &server_addr.sin_addr) <= 0) {
        close(sock);
        return 1.0;
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        return 1.0;
    }

    // Send test data
    size_t sent = 0;
    while (sent < test_size) {
        ssize_t result = send(sock, test_data.data() + sent, test_size - sent, 0);
        if (result <= 0) break;
        sent += result;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    close(sock);

    if (duration.count() > 0) {
        double bandwidth_bps = (sent * 8.0 * 1000000.0) / duration.count();
        return bandwidth_bps / (1024.0 * 1024.0); // Convert to Mbps
    }

    return 1.0;
}

int EnhancedFileTransferManager::measurePacketLoss(const std::string& targetIp) {
    // Simplified packet loss test - send multiple pings
    const int num_pings = 10;
    int lost_packets = 0;

    for (int i = 0; i < num_pings; ++i) {
        double latency = measureLatency(targetIp);
        if (latency > 500.0) { // Consider >500ms as lost
            lost_packets++;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return (lost_packets * 100) / num_pings;
}

bool EnhancedFileTransferManager::initializeEncryption() {
    if (m_encryptionInitialized) return true;

    // Generate random AES key and IV
    m_aesKey.resize(32); // 256-bit key
    m_aesIV.resize(16);  // 128-bit IV

    if (RAND_bytes(m_aesKey.data(), m_aesKey.size()) != 1) {
        std::cerr << "Failed to generate AES key" << std::endl;
        return false;
    }

    if (RAND_bytes(m_aesIV.data(), m_aesIV.size()) != 1) {
        std::cerr << "Failed to generate AES IV" << std::endl;
        return false;
    }

    m_encryptionInitialized = true;
    return true;
}

bool EnhancedFileTransferManager::initializeConnectionPool(size_t poolSize) {
    std::lock_guard<std::mutex> lock(m_poolMutex);

    m_connectionPool.clear();
    m_connectionPool.reserve(poolSize);

    for (size_t i = 0; i < poolSize; ++i) {
        ConnectionInfo conn;
        conn.socket_fd = -1;
        conn.in_use = false;
        conn.last_used = std::chrono::steady_clock::now();
        m_connectionPool.push_back(conn);
    }

    return true;
}

void EnhancedFileTransferManager::cleanupConnectionPool() {
    std::lock_guard<std::mutex> lock(m_poolMutex);

    for (auto& conn : m_connectionPool) {
        if (conn.socket_fd >= 0) {
#ifdef _WIN32
            closesocket(conn.socket_fd);
#else
            close(conn.socket_fd);
#endif
        }
    }

    m_connectionPool.clear();
}

void EnhancedFileTransferManager::resetStatistics() {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    m_statistics = TransferStatistics{};
}

// File transfer utilities implementation
namespace FileTransferUtils {

std::string calculateFileHash(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for hash calculation: " + filePath);
    }

    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    char buffer[8192];
    while (file.read(buffer, sizeof(buffer))) {
        SHA256_Update(&sha256, buffer, file.gcount());
    }
    SHA256_Update(&sha256, buffer, file.gcount());

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);

    file.close();

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

size_t calculateOptimalBufferSize(size_t fileSize, const NetworkConditions& conditions) {
    // Base buffer size
    size_t bufferSize = Constants::Network::FILE_BUFFER_SIZE;

    // Adjust based on file size
    if (fileSize > 100 * 1024 * 1024) { // >100MB
        bufferSize = 256 * 1024; // 256KB
    } else if (fileSize > 10 * 1024 * 1024) { // >10MB
        bufferSize = 128 * 1024; // 128KB
    }

    // Adjust based on network conditions
    if (conditions.bandwidth_mbps > 50.0) {
        bufferSize *= 2; // Double buffer for high bandwidth
    } else if (conditions.bandwidth_mbps < 5.0) {
        bufferSize /= 2; // Half buffer for low bandwidth
    }

    if (conditions.latency_ms > 100.0) {
        bufferSize *= 2; // Larger buffer for high latency
    }

    // Set reasonable bounds
    bufferSize = std::max<size_t>(8192, std::min<size_t>(bufferSize, 1024 * 1024));

    return bufferSize;
}

} // namespace FileTransferUtils
