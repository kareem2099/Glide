#include "file_transfer_improvements.h"
#include "file_transfer_server.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <chrono>
#include <algorithm>
#include <iomanip>

// Test program for improved file transfer system
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Enhanced File Transfer Test Program" << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "Usage: " << argv[0] << " <command> [options]" << std::endl;
        std::cout << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  analyze <target_ip>     - Analyze network conditions" << std::endl;
        std::cout << "  send <file> <target_ip> - Send file with auto-optimization" << std::endl;
        std::cout << "  send_udp <file> <target_ip> - Send small file via UDP" << std::endl;
        std::cout << "  send_parallel <file> <target_ip> [threads] - Send with parallel TCP" << std::endl;
        std::cout << "  receive [directory]     - Start enhanced receive server" << std::endl;
        std::cout << "  benchmark <file> <target_ip> - Run performance benchmarks" << std::endl;
        std::cout << "  stats                   - Show transfer statistics" << std::endl;
        std::cout << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  " << argv[0] << " analyze 192.168.1.100" << std::endl;
        std::cout << "  " << argv[0] << " send document.pdf 192.168.1.100" << std::endl;
        std::cout << "  " << argv[0] << " send_parallel large_file.zip 192.168.1.100 8" << std::endl;
        std::cout << "  " << argv[0] << " receive /tmp/received_files" << std::endl;
        return 1;
    }

    std::string command = argv[1];
    EnhancedFileTransferManager transferManager;

    // Progress callback with enhanced information
    auto progressCallback = [](const FileTransferProgress& progress) {
        std::cout << "\r\033[K" << "Progress: " << progress.percentage << "% ("
                  << progress.bytesTransferred << "/" << progress.fileSize << " bytes) - "
                  << (progress.bytesPerSecond / 1024) << " KB/s - "
                  << progress.status;

        if (progress.isComplete) {
            std::cout << std::endl << "✓ Transfer completed!" << std::endl;
        }
        std::cout.flush();
    };

    if (command == "analyze") {
        if (argc < 3) {
            std::cout << "Error: Target IP required for analysis" << std::endl;
            return 1;
        }

        std::string targetIp = argv[2];
        std::cout << "Analyzing network conditions to " << targetIp << "..." << std::endl;

        NetworkConditions conditions = transferManager.analyzeNetworkConditions(targetIp);
        std::cout << "Network Analysis Results:" << std::endl;
        std::cout << "  " << conditions.to_string() << std::endl;

        // Recommend optimal transfer mode
        std::cout << "Recommended transfer modes:" << std::endl;
        if (conditions.bandwidth_mbps > 10.0 && !conditions.is_congested) {
            std::cout << "  ✓ Parallel TCP (best for large files)" << std::endl;
        }
        if (conditions.latency_ms < 50.0 && conditions.packet_loss_percent < 2) {
            std::cout << "  ✓ UDP Accelerated (best for small files)" << std::endl;
        }
        if (conditions.is_congested || conditions.latency_ms > 100.0) {
            std::cout << "  ✓ Standard TCP (most reliable)" << std::endl;
        }

    } else if (command == "send") {
        if (argc < 4) {
            std::cout << "Error: File path and target IP required" << std::endl;
            return 1;
        }

        std::string filePath = argv[2];
        std::string targetIp = argv[3];

        std::cout << "Starting enhanced file transfer..." << std::endl;
        std::cout << "File: " << filePath << std::endl;
        std::cout << "Target: " << targetIp << std::endl;

        auto startTime = std::chrono::high_resolution_clock::now();
        bool success = transferManager.sendFileAdvanced(targetIp, filePath, progressCallback,
                                                       TransferMode::HYBRID_ADAPTIVE, true, 3);
        auto endTime = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        if (success) {
            std::cout << "File transfer completed successfully!" << std::endl;
            std::cout << "Total time: " << duration.count() << "ms" << std::endl;

            // Show statistics
            TransferStatistics stats = transferManager.getStatistics();
            std::cout << "Transfer Statistics: " << stats.to_string() << std::endl;
            return 0;
        } else {
            std::cout << "File transfer failed!" << std::endl;
            return 1;
        }

    } else if (command == "send_udp") {
        if (argc < 4) {
            std::cout << "Error: File path and target IP required" << std::endl;
            return 1;
        }

        std::string filePath = argv[2];
        std::string targetIp = argv[3];

        std::cout << "Starting UDP accelerated transfer..." << std::endl;
        std::cout << "File: " << filePath << std::endl;
        std::cout << "Target: " << targetIp << std::endl;

        auto startTime = std::chrono::high_resolution_clock::now();
        bool success = transferManager.sendFileUDP(targetIp, filePath, progressCallback);
        auto endTime = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        if (success) {
            std::cout << "UDP transfer completed successfully!" << std::endl;
            std::cout << "Total time: " << duration.count() << "ms" << std::endl;
            return 0;
        } else {
            std::cout << "UDP transfer failed!" << std::endl;
            return 1;
        }

    } else if (command == "send_parallel") {
        if (argc < 4) {
            std::cout << "Error: File path and target IP required" << std::endl;
            return 1;
        }

        std::string filePath = argv[2];
        std::string targetIp = argv[3];
        int numThreads = (argc > 4) ? std::atoi(argv[4]) : 4;

        std::cout << "Starting parallel TCP transfer..." << std::endl;
        std::cout << "File: " << filePath << std::endl;
        std::cout << "Target: " << targetIp << std::endl;
        std::cout << "Threads: " << numThreads << std::endl;

        auto startTime = std::chrono::high_resolution_clock::now();
        bool success = transferManager.sendFileParallel(targetIp, filePath, progressCallback, numThreads);
        auto endTime = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        if (success) {
            std::cout << "Parallel transfer completed successfully!" << std::endl;
            std::cout << "Total time: " << duration.count() << "ms" << std::endl;
            return 0;
        } else {
            std::cout << "Parallel transfer failed!" << std::endl;
            return 1;
        }

    } else if (command == "receive") {
        std::string outputDir = (argc >= 3) ? argv[2] : ".";

        std::cout << "Starting enhanced file receive server..." << std::endl;
        std::cout << "Output directory: " << outputDir << std::endl;
        std::cout << "Port: " << Constants::Network::TCP_PORT << std::endl;
        std::cout << "Features: Network analysis, encryption, verification" << std::endl;

        // Start server in a separate thread
        std::atomic<bool> serverRunning(true);
        std::thread serverThread([&]() {
            FileTransferServer::start_enhanced_file_transfer_server();
        });

        std::cout << "Server started. Press Ctrl+C to stop..." << std::endl;

        // Wait for server thread
        serverThread.join();

        return 0;

    } else if (command == "benchmark") {
        if (argc < 4) {
            std::cout << "Error: File path and target IP required for benchmark" << std::endl;
            return 1;
        }

        std::string filePath = argv[2];
        std::string targetIp = argv[3];

        std::cout << "Running File Transfer Benchmarks" << std::endl;
        std::cout << "================================" << std::endl;
        std::cout << "File: " << filePath << std::endl;
        std::cout << "Target: " << targetIp << std::endl;

        // Test different transfer modes
        struct BenchmarkResult {
            std::string mode;
            bool success;
            std::chrono::milliseconds duration;
            double speed_mbps;
        };

        std::vector<BenchmarkResult> results;

        // Test standard TCP
        std::cout << "Testing Standard TCP..." << std::endl;
        auto startTime = std::chrono::high_resolution_clock::now();
        bool success = transferManager.sendFileAdvanced(targetIp, filePath, nullptr,
                                                       TransferMode::TCP_STANDARD, false, 1);
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        size_t fileSize = transferManager.getFileSize(filePath);
        results.push_back({
            "Standard TCP",
            success,
            duration,
            success ? (fileSize * 8.0) / (duration.count() * 1000000.0) : 0.0
        });

        // Test UDP (if file is small enough)
        if (fileSize < 1024 * 1024) { // < 1MB
            std::cout << "Testing UDP Accelerated..." << std::endl;
            startTime = std::chrono::high_resolution_clock::now();
            success = transferManager.sendFileUDP(targetIp, filePath, nullptr);
            endTime = std::chrono::high_resolution_clock::now();
            duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

            results.push_back({
                "UDP Accelerated",
                success,
                duration,
                success ? (fileSize * 8.0) / (duration.count() * 1000000.0) : 0.0
            });
        }

        // Test parallel TCP
        std::cout << "Testing Parallel TCP..." << std::endl;
        startTime = std::chrono::high_resolution_clock::now();
        success = transferManager.sendFileParallel(targetIp, filePath, nullptr, 4);
        endTime = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        results.push_back({
            "Parallel TCP (4 threads)",
            success,
            duration,
            success ? (fileSize * 8.0) / (duration.count() * 1000000.0) : 0.0
        });

        // Display results
        std::cout << std::endl << "Benchmark Results:" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        std::cout << std::left << std::setw(25) << "Mode"
                  << std::setw(10) << "Success"
                  << std::setw(15) << "Time (ms)"
                  << std::setw(15) << "Speed (Mbps)" << std::endl;
        std::cout << std::string(60, '-') << std::endl;

        for (const auto& result : results) {
            std::cout << std::left << std::setw(25) << result.mode
                      << std::setw(10) << (result.success ? "✓" : "✗")
                      << std::setw(15) << result.duration.count()
                      << std::setw(15) << std::fixed << std::setprecision(2) << result.speed_mbps
                      << std::endl;
        }

        // Find best performing method
        auto best = std::max_element(results.begin(), results.end(),
            [](const BenchmarkResult& a, const BenchmarkResult& b) {
                return a.success && b.success ? a.speed_mbps < b.speed_mbps : !a.success;
            });

        if (best != results.end() && best->success) {
            std::cout << std::endl << "🏆 Best Performance: " << best->mode
                      << " (" << best->speed_mbps << " Mbps)" << std::endl;
        }

    } else if (command == "stats") {
        TransferStatistics stats = transferManager.getStatistics();
        std::cout << "File Transfer Statistics:" << std::endl;
        std::cout << "========================" << std::endl;
        std::cout << stats.to_string() << std::endl;

    } else {
        std::cout << "Error: Unknown command '" << command << "'" << std::endl;
        std::cout << "Run '" << argv[0] << "' without arguments for help" << std::endl;
        return 1;
    }

    return 0;
}
