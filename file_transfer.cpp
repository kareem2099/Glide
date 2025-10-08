#include "file_transfer.h"
#include <cstring> // For memset
#include <thread> // For usleep on Linux
#include <algorithm> // For std::min
#include <sstream> // For stringstream
#include <iomanip> // For hex formatting
#include <openssl/sha.h> // For SHA-256 checksums
#include <zlib.h> // For compression
#include <chrono> // For timing
#include <random> // For generating job IDs
#include <queue> // For priority queue
#include <map> // For state data storage


#ifdef _WIN32
void send_file_tcp(const std::string& serverIp, const std::string& filePath) {
    SOCKET fileSocket;
    sockaddr_in serverAddr;

    fileSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fileSocket == INVALID_SOCKET) {
        std::cerr << "TCP file socket creation failed: " << WSAGetLastError() << std::endl;
        return;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(Constants::Network::TCP_PORT);
    if (inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr) != 1) {
        std::cerr << "Invalid server IP address for file transfer: " << serverIp << std::endl;
        closesocket(fileSocket);
        return;
    }

    if (connect(fileSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "TCP file connect failed: " << WSAGetLastError() << std::endl;
        closesocket(fileSocket);
        return;
    }
    std::cout << "Connected to server for file transfer." << std::endl;

    std::filesystem::path p(filePath);
    std::string filename = p.filename().string();

    // Send filename
    if (send(fileSocket, filename.c_str(), filename.length(), 0) == SOCKET_ERROR) {
        std::cerr << "Failed to send filename: " << WSAGetLastError() << std::endl;
        closesocket(fileSocket);
        return;
    }
    // Add a small delay or confirmation if needed to ensure server is ready for file content
    Sleep(100); 

    std::ifstream inFile(filePath, std::ios::binary);
    if (!inFile.is_open()) {
        std::cerr << "Failed to open file for reading: " << filePath << std::endl;
        closesocket(fileSocket);
        return;
    }

    char buffer[Constants::Network::BUFFER_SIZE];
    while (!inFile.eof()) {
        inFile.read(buffer, Constants::Network::BUFFER_SIZE);
        int bytesRead = inFile.gcount();
        if (bytesRead > 0) {
            if (send(fileSocket, buffer, bytesRead, 0) == SOCKET_ERROR) {
                std::cerr << "Failed to send file data: " << WSAGetLastError() << std::endl;
                break;
            }
        }
    }

    if (inFile.eof()) {
        std::cout << "File sent successfully: " << filePath << std::endl;
    } else {
        std::cerr << "Error reading file: " << filePath << std::endl;
    }

    inFile.close();
    closesocket(fileSocket);
}

#else // Linux X11 specific
void send_file_tcp_linux(const std::string& serverIp, const std::string& filePath) {
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "TCP file socket creation error" << std::endl;
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(Constants::Network::TCP_PORT);

    if (inet_pton(AF_INET, serverIp.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid server IP address for file transfer: " << serverIp << std::endl;
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("TCP file connect failed");
        close(sock);
        return;
    }
    std::cout << "Connected to server for file transfer." << std::endl;

    std::filesystem::path p(filePath);
    std::string filename = p.filename().string();

    // Send filename
    if (send(sock, filename.c_str(), filename.length(), 0) == -1) {
        perror("Failed to send filename");
        close(sock);
        return;
    }
    usleep(100000); // 100ms delay

    std::ifstream inFile(filePath, std::ios::binary);
    if (!inFile.is_open()) {
        std::cerr << "Failed to open file for reading: " << filePath << std::endl;
        close(sock);
        return;
    }

    char buffer[Constants::Network::BUFFER_SIZE];
    while (!inFile.eof()) {
        inFile.read(buffer, Constants::Network::BUFFER_SIZE);
        ssize_t bytesRead = inFile.gcount();
        if (bytesRead > 0) {
            if (send(sock, buffer, bytesRead, 0) == -1) {
                perror("Failed to send file data");
                break;
            }
        }
    }

    if (inFile.eof()) {
        std::cout << "File sent successfully: " << filePath << std::endl;
    } else {
        std::cerr << "Error reading file: " << filePath << std::endl;
    }

    inFile.close();
    close(sock);
}
#endif // _WIN32

// FileTransferManager implementation
FileTransferManager::FileTransferManager()
    : m_lastProgressUpdate(std::chrono::steady_clock::now())
    , m_lastBytesTransferred(0)
    , m_cancelled(false)
{
}

FileTransferManager::~FileTransferManager()
{
    // Ensure any ongoing transfer is cancelled on destruction
    cancelTransfer();
}

void FileTransferManager::cancelTransfer()
{
    m_cancelled.store(true);
}

bool FileTransferManager::isCancelled() const
{
    return m_cancelled.load();
}

bool FileTransferManager::sendFile(const std::string& serverIp,
                                  const std::string& filePath,
                                  ProgressCallback progressCallback,
                                  bool enableCompression,
                                  bool enableResume)
{
    try {
        // Validate input file
        if (!std::filesystem::exists(filePath)) {
            std::cerr << "Error: File does not exist: " << filePath << std::endl;
            return false;
        }

        size_t fileSize = getFileSize(filePath);
        if (fileSize == 0) {
            std::cerr << "Error: File is empty: " << filePath << std::endl;
            return false;
        }

        // Calculate file checksum
        std::string checksum = calculateFileChecksum(filePath);

        // Initialize progress
        FileTransferProgress progress;
        progress.filename = std::filesystem::path(filePath).filename().string();
        progress.fileSize = fileSize;
        progress.bytesTransferred = 0;
        progress.bytesPerSecond = 0;
        progress.percentage = 0;
        progress.isComplete = false;
        progress.status = "Starting transfer...";

        if (progressCallback) {
            progressCallback(progress);
        }

        // Try to send file with resume capability if enabled
        if (enableResume) {
            if (sendFileWithResume(serverIp, filePath, progressCallback, enableCompression)) {
                return true;
            }
        }

        // Fallback to chunked transfer
        return sendFileChunked(serverIp, filePath, progressCallback, enableCompression);

    } catch (const std::exception& e) {
        std::cerr << "Error in sendFile: " << e.what() << std::endl;
        return false;
    }
}

bool FileTransferManager::sendFileWithResume(const std::string& serverIp,
                                           const std::string& filePath,
                                           ProgressCallback progressCallback,
                                           bool enableCompression)
{
    // TODO: Implement resume capability
    // For now, fall back to chunked transfer
    return false;
}

bool FileTransferManager::sendFileChunked(const std::string& serverIp,
                                        const std::string& filePath,
                                        ProgressCallback progressCallback,
                                        bool enableCompression)
{
    int sock = 0;
    struct sockaddr_in serv_addr;

#ifdef _WIN32
    SOCKET fileSocket;
    sockaddr_in serverAddr;

    fileSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fileSocket == INVALID_SOCKET) {
        std::cerr << "TCP file socket creation failed: " << WSAGetLastError() << std::endl;
        return false;
    }
    sock = fileSocket;
#else
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "TCP file socket creation error" << std::endl;
        return false;
    }
#endif

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(Constants::Network::TCP_PORT);

    if (inet_pton(AF_INET, serverIp.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid server IP address for file transfer: " << serverIp << std::endl;
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return false;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
#ifdef _WIN32
        std::cerr << "TCP file connect failed: " << WSAGetLastError() << std::endl;
        closesocket(sock);
#else
        perror("TCP file connect failed");
        close(sock);
#endif
        return false;
    }

    std::cout << "Connected to server for enhanced file transfer." << std::endl;

    try {
        // Get file information
        std::filesystem::path p(filePath);
        std::string filename = p.filename().string();
        size_t fileSize = getFileSize(filePath);
        std::string checksum = calculateFileChecksum(filePath);

        // Send transfer metadata
        FileTransferMetadata metadata;
        metadata.filename = filename;
        metadata.fileSize = fileSize;
        metadata.checksum = checksum;
        metadata.compressed = false;
        metadata.compressedSize = 0;
        metadata.lastModified = getFileLastModified(filePath);

        // Send metadata as JSON-like string
        std::stringstream ss;
        ss << "FILE_START:" << metadata.filename << ":"
           << metadata.fileSize << ":" << metadata.checksum << ":"
           << (metadata.compressed ? "1" : "0") << ":" << metadata.compressedSize << ":"
           << metadata.lastModified;

        if (!sendString(sock, ss.str())) {
            throw std::runtime_error("Failed to send file metadata");
        }

        // Wait for server acknowledgment
        std::string response;
        if (!receiveString(sock, response)) {
            throw std::runtime_error("Failed to receive server acknowledgment");
        }

        if (response != "ACK") {
            throw std::runtime_error("Server rejected file transfer: " + response);
        }

        // Open file for reading
        std::ifstream inFile(filePath, std::ios::binary);
        if (!inFile.is_open()) {
            throw std::runtime_error("Failed to open file for reading: " + filePath);
        }

        // Send file data in chunks
        char buffer[Constants::Network::FILE_BUFFER_SIZE];
        size_t totalBytesSent = 0;
        auto startTime = std::chrono::steady_clock::now();

        while (!inFile.eof() && !isCancelled()) {
            inFile.read(buffer, Constants::Network::FILE_BUFFER_SIZE);
            size_t bytesRead = inFile.gcount();

            if (bytesRead > 0) {
                if (!sendData(sock, buffer, bytesRead)) {
                    throw std::runtime_error("Failed to send file data");
                }

                totalBytesSent += bytesRead;

                // Update progress
                if (progressCallback) {
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);

                    if (elapsed.count() > 0) {
                        size_t bytesPerSecond = (totalBytesSent * 1000) / elapsed.count();

                        FileTransferProgress progress;
                        progress.filename = filename;
                        progress.fileSize = fileSize;
                        progress.bytesTransferred = totalBytesSent;
                        progress.bytesPerSecond = bytesPerSecond;
                        progress.percentage = static_cast<int>((totalBytesSent * 100) / fileSize);
                        progress.isComplete = false;
                        progress.status = "Transferring...";

                        progressCallback(progress);
                    }
                }
            }
        }

        // Check if transfer was cancelled
        if (isCancelled()) {
            throw std::runtime_error("Transfer was cancelled by user");
        }

        // Send completion signal
        if (!sendString(sock, "FILE_END")) {
            throw std::runtime_error("Failed to send completion signal");
        }

        // Wait for final acknowledgment
        if (!receiveString(sock, response)) {
            throw std::runtime_error("Failed to receive final acknowledgment");
        }

        if (response == "SUCCESS") {
            // Final progress update
            if (progressCallback) {
                FileTransferProgress progress;
                progress.filename = filename;
                progress.fileSize = fileSize;
                progress.bytesTransferred = totalBytesSent;
                progress.bytesPerSecond = 0;
                progress.percentage = 100;
                progress.isComplete = true;
                progress.status = "Transfer completed successfully";

                progressCallback(progress);
            }

            std::cout << "File sent successfully: " << filePath << std::endl;
            std::cout << "Total bytes sent: " << totalBytesSent << std::endl;
        } else {
            throw std::runtime_error("Transfer failed: " + response);
        }

        inFile.close();
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error during file transfer: " << e.what() << std::endl;
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return false;
    }
}

bool FileTransferManager::receiveFile(int clientSocket,
                                    const std::string& outputDirectory,
                                    ProgressCallback progressCallback)
{
    return receiveFileWithVerification(clientSocket, outputDirectory, progressCallback);
}

bool FileTransferManager::receiveFileWithVerification(int clientSocket,
                                                    const std::string& outputDirectory,
                                                    ProgressCallback progressCallback)
{
    try {
        // Receive file metadata
        std::string metadataStr;
        if (!receiveString(clientSocket, metadataStr)) {
            throw std::runtime_error("Failed to receive file metadata");
        }

        if (metadataStr.substr(0, 11) != "FILE_START:") {
            throw std::runtime_error("Invalid metadata format");
        }

        // Parse metadata (simplified parsing)
        size_t pos1 = metadataStr.find(':', 11);
        size_t pos2 = metadataStr.find(':', pos1 + 1);
        size_t pos3 = metadataStr.find(':', pos2 + 1);
        size_t pos4 = metadataStr.find(':', pos3 + 1);
        size_t pos5 = metadataStr.find(':', pos4 + 1);

        std::string filename = metadataStr.substr(11, pos1 - 11);
        size_t fileSize = std::stoull(metadataStr.substr(pos1 + 1, pos2 - pos1 - 1));
        std::string checksum = metadataStr.substr(pos2 + 1, pos3 - pos2 - 1);

        // Send acknowledgment
        if (!sendString(clientSocket, "ACK")) {
            throw std::runtime_error("Failed to send acknowledgment");
        }

        // Create output file path
        std::string outputPath = outputDirectory.empty() ?
            filename : (outputDirectory + "/" + filename);

        // Open file for writing
        std::ofstream outFile(outputPath, std::ios::binary);
        if (!outFile.is_open()) {
            throw std::runtime_error("Failed to open file for writing: " + outputPath);
        }

        // Receive file data
        char buffer[Constants::Network::FILE_BUFFER_SIZE];
        size_t totalBytesReceived = 0;
        auto startTime = std::chrono::steady_clock::now();

        while (totalBytesReceived < fileSize && !isCancelled()) {
            size_t remaining = fileSize - totalBytesReceived;
            size_t toRead = std::min(remaining, (size_t)Constants::Network::FILE_BUFFER_SIZE);

            size_t bytesReceived = 0;
            if (!receiveData(clientSocket, buffer, toRead, &bytesReceived)) {
                throw std::runtime_error("Failed to receive file data");
            }

            if (bytesReceived == 0) {
                break; // Connection closed
            }

            outFile.write(buffer, bytesReceived);
            totalBytesReceived += bytesReceived;

            // Update progress
            if (progressCallback) {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);

                if (elapsed.count() > 0) {
                    size_t bytesPerSecond = (totalBytesReceived * 1000) / elapsed.count();

                    FileTransferProgress progress;
                    progress.filename = filename;
                    progress.fileSize = fileSize;
                    progress.bytesTransferred = totalBytesReceived;
                    progress.bytesPerSecond = bytesPerSecond;
                    progress.percentage = static_cast<int>((totalBytesReceived * 100) / fileSize);
                    progress.isComplete = false;
                    progress.status = "Receiving...";

                    progressCallback(progress);
                }
            }
        }

        // Check if transfer was cancelled
        if (isCancelled()) {
            throw std::runtime_error("Transfer was cancelled by user");
        }

        outFile.close();

        // Verify file integrity
        std::string receivedChecksum = calculateFileChecksum(outputPath);
        if (receivedChecksum != checksum) {
            throw std::runtime_error("File integrity check failed");
        }

        // Send success acknowledgment
        if (!sendString(clientSocket, "SUCCESS")) {
            throw std::runtime_error("Failed to send success acknowledgment");
        }

        // Final progress update
        if (progressCallback) {
            FileTransferProgress progress;
            progress.filename = filename;
            progress.fileSize = fileSize;
            progress.bytesTransferred = totalBytesReceived;
            progress.bytesPerSecond = 0;
            progress.percentage = 100;
            progress.isComplete = true;
            progress.status = "Transfer completed successfully";

            progressCallback(progress);
        }

        std::cout << "File received successfully: " << outputPath << std::endl;
        std::cout << "Total bytes received: " << totalBytesReceived << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error receiving file: " << e.what() << std::endl;
        return false;
    }
}

std::string FileTransferManager::calculateFileChecksum(const std::string& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for checksum calculation: " + filePath);
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

    // Convert to hex string
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

std::vector<uint8_t> FileTransferManager::compressFileData(const std::vector<uint8_t>& data)
{
    if (data.size() < Constants::FileTransfer::COMPRESSION_THRESHOLD) {
        // Don't compress small files
        return data;
    }

    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    if (deflateInit(&zs, Z_DEFAULT_COMPRESSION) != Z_OK) {
        std::cerr << "Failed to initialize zlib compression" << std::endl;
        return data;
    }

    zs.next_in = const_cast<Bytef*>(data.data());
    zs.avail_in = data.size();

    std::vector<uint8_t> compressedData;
    int ret;
    char outbuffer[32768];

    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = deflate(&zs, Z_FINISH);

        if (compressedData.size() < zs.total_out) {
            compressedData.insert(compressedData.end(), outbuffer, outbuffer + (zs.total_out - compressedData.size()));
        }
    } while (ret == Z_OK);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END) {
        std::cerr << "Compression failed" << std::endl;
        return data;
    }

    // Only return compressed data if it's actually smaller
    if (compressedData.size() < data.size()) {
        std::cout << "Compression successful: " << data.size() << " -> " << compressedData.size() << " bytes" << std::endl;
        return compressedData;
    } else {
        std::cout << "Compression not beneficial, using original data" << std::endl;
        return data;
    }
}

std::vector<uint8_t> FileTransferManager::decompressFileData(const std::vector<uint8_t>& compressedData)
{
    if (compressedData.empty()) {
        return compressedData;
    }

    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    if (inflateInit(&zs) != Z_OK) {
        std::cerr << "Failed to initialize zlib decompression" << std::endl;
        return compressedData;
    }

    zs.next_in = const_cast<Bytef*>(compressedData.data());
    zs.avail_in = compressedData.size();

    std::vector<uint8_t> decompressedData;
    int ret;
    char outbuffer[32768];

    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = inflate(&zs, 0);

        if (decompressedData.size() < zs.total_out) {
            decompressedData.insert(decompressedData.end(), outbuffer, outbuffer + (zs.total_out - decompressedData.size()));
        }
    } while (ret == Z_OK);

    inflateEnd(&zs);

    if (ret != Z_STREAM_END) {
        std::cerr << "Decompression failed" << std::endl;
        return compressedData;
    }

    std::cout << "Decompression successful: " << compressedData.size() << " -> " << decompressedData.size() << " bytes" << std::endl;
    return decompressedData;
}

std::string FileTransferManager::getFileLastModified(const std::string& filePath)
{
    auto ftime = std::filesystem::last_write_time(filePath);
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());

    std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
    return std::to_string(tt);
}

bool FileTransferManager::validateFileTransfer(const std::string& filePath, const std::string& expectedChecksum)
{
    try {
        std::string actualChecksum = calculateFileChecksum(filePath);
        return actualChecksum == expectedChecksum;
    } catch (const std::exception& e) {
        std::cerr << "Error validating file transfer: " << e.what() << std::endl;
        return false;
    }
}

size_t FileTransferManager::getFileSize(const std::string& filePath)
{
    return std::filesystem::file_size(filePath);
}

bool FileTransferManager::sendData(int socket, const void* data, size_t size)
{
#ifdef _WIN32
    return send(socket, (const char*)data, size, 0) != SOCKET_ERROR;
#else
    return send(socket, data, size, 0) != -1;
#endif
}

bool FileTransferManager::receiveData(int socket, void* buffer, size_t size, size_t* bytesReceived)
{
#ifdef _WIN32
    int result = recv(socket, (char*)buffer, size, 0);
    if (bytesReceived) *bytesReceived = (result > 0) ? result : 0;
    return result > 0;
#else
    ssize_t result = recv(socket, buffer, size, 0);
    if (bytesReceived) *bytesReceived = (result > 0) ? result : 0;
    return result > 0;
#endif
}

bool FileTransferManager::sendString(int socket, const std::string& str)
{
    uint32_t length = htonl(str.length());
    if (!sendData(socket, &length, sizeof(length))) {
        return false;
    }
    return sendData(socket, str.c_str(), str.length());
}

bool FileTransferManager::receiveString(int socket, std::string& str, size_t maxLength)
{
    uint32_t length;
    if (!receiveData(socket, &length, sizeof(length))) {
        return false;
    }
    length = ntohl(length);

    if (length > maxLength) {
        return false;
    }

    str.resize(length);
    size_t bytesReceived = 0;
    return receiveData(socket, &str[0], length, &bytesReceived) && bytesReceived == length;
}
 
// Queue Management Methods
std::string FileTransferManager::generateJobId()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 16; i++) {
        ss << dis(gen);
    }
    return ss.str();
}

bool FileTransferManager::addTransferToQueue(const std::string& serverIp,
                                           const std::string& filePath,
                                           const std::string& outputDirectory,
                                           ProgressCallback progressCallback,
                                           bool enableCompression,
                                           bool enableResume,
                                           int priority,
                                           bool isUpload)
{
    std::lock_guard<std::mutex> lock(m_queueMutex);

    TransferJob job;
    job.serverIp = serverIp;
    job.filePath = filePath;
    job.outputDirectory = outputDirectory;
    job.progressCallback = progressCallback;
    job.enableCompression = enableCompression;
    job.enableResume = enableResume;
    job.priority = priority;
    job.jobId = generateJobId();
    job.isUpload = isUpload;

    m_transferQueue.push_back(job);

    // Sort by priority (higher priority first)
    std::sort(m_transferQueue.begin(), m_transferQueue.end(),
              [](const TransferJob& a, const TransferJob& b) {
                  return a.priority > b.priority;
              });

    std::cout << "Added transfer job to queue: " << job.jobId
              << " (Priority: " << job.priority << ")" << std::endl;

    return true;
}

bool FileTransferManager::processQueue()
{
    if (m_queueProcessing.load()) {
        std::cout << "Queue is already being processed" << std::endl;
        return false;
    }

    m_queueProcessing.store(true);

    while (!m_transferQueue.empty() && !isCancelled()) {
        std::lock_guard<std::mutex> lock(m_queueMutex);

        if (m_transferQueue.empty()) {
            break;
        }

        TransferJob job = m_transferQueue.front();
        m_transferQueue.erase(m_transferQueue.begin());
        m_currentJobId = job.jobId;

        std::cout << "Processing transfer job: " << job.jobId << std::endl;

        bool success = false;
        if (job.isUpload) {
            success = sendFile(job.serverIp, job.filePath, job.progressCallback,
                             job.enableCompression, job.enableResume);
        } else {
            // For downloads, we would need a server socket
            // This is a simplified implementation
            std::cout << "Download jobs require server mode" << std::endl;
            success = false;
        }

        if (success) {
            std::cout << "Transfer job completed successfully: " << job.jobId << std::endl;
        } else {
            std::cout << "Transfer job failed: " << job.jobId << std::endl;
        }

        m_currentJobId.clear();
    }

    m_queueProcessing.store(false);
    return true;
}

void FileTransferManager::clearQueue()
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_transferQueue.clear();
    std::cout << "Transfer queue cleared" << std::endl;
}

size_t FileTransferManager::getQueueSize()
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_transferQueue.size();
}

std::string FileTransferManager::getCurrentJobId() const
{
    return m_currentJobId;
}

bool FileTransferManager::isQueueProcessing() const
{
    return m_queueProcessing.load();
}

// State Persistence Methods
void FileTransferManager::setStateFilePath(const std::string& filePath)
{
    m_transferStateFile = filePath;
}

bool FileTransferManager::saveTransferState(const std::string& sessionId)
{
    std::lock_guard<std::mutex> lock(m_stateMutex);

    std::string session = sessionId.empty() ? generateJobId() : sessionId;

    // Create state directory if it doesn't exist
    std::filesystem::path stateDir = std::filesystem::path(m_transferStateFile).parent_path();
    if (!stateDir.empty() && !std::filesystem::exists(stateDir)) {
        std::filesystem::create_directories(stateDir);
    }

    // Save current transfer state
    std::ofstream stateFile(m_transferStateFile);
    if (!stateFile.is_open()) {
        std::cerr << "Failed to open state file for writing: " << m_transferStateFile << std::endl;
        return false;
    }

    stateFile << "session_id=" << session << std::endl;
    stateFile << "file_path=" << m_currentTransferState.filePath << std::endl;
    stateFile << "server_ip=" << m_currentTransferState.serverIp << std::endl;
    stateFile << "file_size=" << m_currentTransferState.fileSize << std::endl;
    stateFile << "bytes_transferred=" << m_currentTransferState.bytesTransferred << std::endl;
    stateFile << "checksum=" << m_currentTransferState.checksum << std::endl;
    stateFile << "is_upload=" << (m_currentTransferState.isUpload ? "1" : "0") << std::endl;
    stateFile << "enable_compression=" << (m_currentTransferState.enableCompression ? "1" : "0") << std::endl;
    stateFile << "status=" << m_currentTransferState.status << std::endl;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_currentTransferState.startTime);
    stateFile << "elapsed_seconds=" << elapsed.count() << std::endl;

    stateFile.close();
    std::cout << "Transfer state saved: " << session << std::endl;
    return true;
}

bool FileTransferManager::loadTransferState(const std::string& sessionId)
{
    std::lock_guard<std::mutex> lock(m_stateMutex);

    std::ifstream stateFile(m_transferStateFile);
    if (!stateFile.is_open()) {
        std::cerr << "Failed to open state file for reading: " << m_transferStateFile << std::endl;
        return false;
    }

    std::string line;
    std::map<std::string, std::string> stateData;

    while (std::getline(stateFile, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            stateData[key] = value;
        }
    }

    stateFile.close();

    // Load state data
    m_currentTransferState.sessionId = stateData["session_id"];
    m_currentTransferState.filePath = stateData["file_path"];
    m_currentTransferState.serverIp = stateData["server_ip"];
    m_currentTransferState.fileSize = std::stoull(stateData["file_size"]);
    m_currentTransferState.bytesTransferred = std::stoull(stateData["bytes_transferred"]);
    m_currentTransferState.checksum = stateData["checksum"];
    m_currentTransferState.isUpload = (stateData["is_upload"] == "1");
    m_currentTransferState.enableCompression = (stateData["enable_compression"] == "1");
    m_currentTransferState.status = stateData["status"];

    // Calculate start time based on elapsed seconds
    size_t elapsedSeconds = std::stoull(stateData["elapsed_seconds"]);
    m_currentTransferState.startTime = std::chrono::steady_clock::now() - std::chrono::seconds(elapsedSeconds);

    std::cout << "Transfer state loaded: " << sessionId << std::endl;
    return true;
}

bool FileTransferManager::resumeTransfer(const std::string& sessionId, ProgressCallback progressCallback)
{
    if (!loadTransferState(sessionId)) {
        return false;
    }

    std::cout << "Resuming transfer from state:" << std::endl;
    std::cout << "  File: " << m_currentTransferState.filePath << std::endl;
    std::cout << "  Progress: " << m_currentTransferState.bytesTransferred << "/" << m_currentTransferState.fileSize << " bytes" << std::endl;
    std::cout << "  Status: " << m_currentTransferState.status << std::endl;

    // Resume the transfer
    if (m_currentTransferState.isUpload) {
        return sendFile(m_currentTransferState.serverIp, m_currentTransferState.filePath,
                       progressCallback, m_currentTransferState.enableCompression, true);
    } else {
        // For downloads, we would need server mode
        std::cout << "Resume download not yet implemented" << std::endl;
        return false;
    }
}

std::vector<std::string> FileTransferManager::getSavedSessions()
{
    std::vector<std::string> sessions;

    // For now, return empty list since we only save one state at a time
    // In a full implementation, we would scan a directory for state files
    if (std::filesystem::exists(m_transferStateFile)) {
        sessions.push_back("current");
    }

    return sessions;
}

bool FileTransferManager::deleteTransferState(const std::string& sessionId)
{
    if (std::filesystem::exists(m_transferStateFile)) {
        if (std::filesystem::remove(m_transferStateFile)) {
            std::cout << "Transfer state deleted: " << sessionId << std::endl;
            return true;
        }
    }
    return false;
}
