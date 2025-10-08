#include "file_transfer.h"
#include "file_transfer_server.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <string>

// Test program for enhanced file transfer system
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <send|receive|queue> <file_path> [server_ip]" << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  " << argv[0] << " send /path/to/file.txt 192.168.1.100" << std::endl;
        std::cout << "  " << argv[0] << " receive /output/directory" << std::endl;
        std::cout << "  " << argv[0] << " queue /path/to/file.txt 192.168.1.100" << std::endl;
        return 1;
    }

    std::string mode = argv[1];
    std::string filePath = argv[2];

    // Progress callback
    auto progressCallback = [](const FileTransferProgress& progress) {
        std::cout << "\rProgress: " << progress.percentage << "% ("
                  << progress.bytesTransferred << "/" << progress.fileSize << " bytes) - "
                  << (progress.bytesPerSecond / 1024) << " KB/s - "
                  << progress.status << std::flush;

        if (progress.isComplete) {
            std::cout << std::endl << "Transfer completed!" << std::endl;
        }
    };

    if (mode == "send") {
        if (argc < 4) {
            std::cout << "Error: Server IP required for send mode" << std::endl;
            return 1;
        }

        std::string serverIp = argv[3];

        std::cout << "Starting enhanced file transfer..." << std::endl;
        std::cout << "File: " << filePath << std::endl;
        std::cout << "Server: " << serverIp << std::endl;
        std::cout << "Port: " << Constants::Network::TCP_PORT << std::endl;

        FileTransferManager transferManager;
        bool success = transferManager.sendFile(serverIp, filePath, progressCallback, true, false);

        if (success) {
            std::cout << "File transfer completed successfully!" << std::endl;
            return 0;
        } else {
            std::cout << "File transfer failed!" << std::endl;
            return 1;
        }
    } else if (mode == "receive") {
        std::string outputDir = (argc >= 4) ? argv[3] : ".";

        std::cout << "Starting enhanced file receive server..." << std::endl;
        std::cout << "Output directory: " << outputDir << std::endl;
        std::cout << "Port: " << Constants::Network::TCP_PORT << std::endl;

        // Start server in a separate thread
        std::atomic<bool> serverRunning(true);
        std::thread serverThread([&]() {
            FileTransferServer::start_enhanced_file_transfer_server();
        });

        std::cout << "Server started. Press Ctrl+C to stop..." << std::endl;

        // Wait for server thread
        serverThread.join();

        return 0;
    } else if (mode == "queue") {
        if (argc < 4) {
            std::cout << "Error: Server IP required for queue mode" << std::endl;
            return 1;
        }

        std::string serverIp = argv[3];

        std::cout << "Enhanced File Transfer Queue System" << std::endl;
        std::cout << "====================================" << std::endl;
        std::cout << "File: " << filePath << std::endl;
        std::cout << "Server: " << serverIp << std::endl;
        std::cout << "Port: " << Constants::Network::TCP_PORT << std::endl;

        FileTransferManager transferManager;

        // Add file to transfer queue with high priority
        std::cout << "Adding file to transfer queue..." << std::endl;
        bool added = transferManager.addTransferToQueue(serverIp, filePath, "",
                                                       progressCallback, true, false, 10, true);

        if (!added) {
            std::cout << "Failed to add transfer to queue!" << std::endl;
            return 1;
        }

        std::cout << "Queue size: " << transferManager.getQueueSize() << std::endl;

        // Process the queue
        std::cout << "Processing transfer queue..." << std::endl;
        bool success = transferManager.processQueue();

        if (success) {
            std::cout << "Queue processing completed successfully!" << std::endl;
            return 0;
        } else {
            std::cout << "Queue processing failed!" << std::endl;
            return 1;
        }
    } else {
        std::cout << "Error: Invalid mode. Use 'send', 'receive', or 'queue'" << std::endl;
        return 1;
    }
}
