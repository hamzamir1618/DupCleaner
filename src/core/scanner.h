#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include <chrono>
#include "dupcleaner/file_entry.h"

namespace dupcleaner {

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

class DirectoryScanner {
public:
    ScanResult scan(const std::filesystem::path& root);
};

} // namespace dupcleaner
