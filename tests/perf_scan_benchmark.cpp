#define DUPCLEANER_TESTING_ENV
#include <gtest/gtest.h>
#include <chrono>
#include <iostream>
#include "gen_test_tree.h"
#include "scanner.h"
#include "duplicate_finder.h"

using namespace dupcleaner;
using namespace dupcleaner::tools;

TEST(PerfScanBenchmark, LargeTreeScan) {
    TreeConfig config;
    config.output_dir = std::filesystem::temp_directory_path() / "dupcleaner_perf_tree";
    config.num_files = 100000;
    config.duplicate_ratio = 0.2;
    config.min_file_size = 1024;
    config.max_file_size = 64 * 1024; // 64KB to limit disk usage while keeping file count high

    std::cout << "[Perf] Generating synthetic tree with " << config.num_files << " files...\n";
    auto start_gen = std::chrono::high_resolution_clock::now();
    uintmax_t total_bytes = generateTestTree(config);
    auto end_gen = std::chrono::high_resolution_clock::now();
    
    double gen_sec = std::chrono::duration<double>(end_gen - start_gen).count();
    std::cout << "[Perf] Generation took " << gen_sec << " seconds. Total size: " << total_bytes / (1024*1024) << " MB\n";

    DirectoryScanner scanner;
    DuplicateFinder finder;

    std::cout << "[Perf] Scanning directory...\n";
    auto start_scan = std::chrono::high_resolution_clock::now();
    auto result = scanner.scan(config.output_dir.string());
    auto end_scan = std::chrono::high_resolution_clock::now();

    std::cout << "[Perf] Finding duplicates...\n";
    auto start_find = std::chrono::high_resolution_clock::now();
    auto duplicates = finder.findExactDuplicates(result.entries);
    auto end_find = std::chrono::high_resolution_clock::now();

    double scan_sec = std::chrono::duration<double>(end_scan - start_scan).count();
    double find_sec = std::chrono::duration<double>(end_find - start_find).count();
    double total_sec = scan_sec + find_sec;

    std::cout << "====================================\n";
    std::cout << "PERFORMANCE BENCHMARK RESULTS\n";
    std::cout << "====================================\n";
    std::cout << "Files Scanned: " << result.entries.size() << "\n";
    std::cout << "Total Time:    " << total_sec << " seconds\n";
    std::cout << "Scan Throughput: " << (result.entries.size() / scan_sec) << " files/sec\n";
    if (total_sec > 0) {
        std::cout << "Total Throughput: " << ((total_bytes / (1024.0 * 1024.0)) / total_sec) << " MB/sec\n";
    }
    std::cout << "====================================\n";

    // Clean up
    std::filesystem::remove_all(config.output_dir);
}
