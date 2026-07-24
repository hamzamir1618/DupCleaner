#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include <chrono>
#include <atomic>
#include "dupcleaner/file_entry.h"

namespace dupcleaner {

// Converts a file path glob pattern (e.g. "*.txt") into a regex string.
std::string globToRegex(const std::string& glob);

struct ScanStats {
    uintmax_t files_visited{0};
    uintmax_t bytes_visited{0};
    uintmax_t directories_visited{0};
    uintmax_t items_skipped{0};
    std::chrono::milliseconds duration{0};
};

struct ScanResult {
    std::vector<FileEntry> entries;
    std::vector<std::string> skipped_paths;
    ScanStats stats;
};

struct ScannerOptions {
    uintmax_t min_size{0};
    std::vector<std::string> exclude_patterns;
    bool no_default_excludes{false};
};

class DirectoryScanner {
public:
    struct ProgressTracker {
        std::atomic<uintmax_t>* files_visited;
        std::atomic<uintmax_t>* bytes_visited;
        ProgressTracker() : files_visited(nullptr), bytes_visited(nullptr) {}
        ProgressTracker(std::atomic<uintmax_t>* f, std::atomic<uintmax_t>* b) : files_visited(f), bytes_visited(b) {}
    };

    ScanResult scan(const std::filesystem::path& root, const ScannerOptions& opts = ScannerOptions{}, std::atomic<bool>* cancelFlag = nullptr, const ProgressTracker& progress = ProgressTracker());
};

} // namespace dupcleaner
