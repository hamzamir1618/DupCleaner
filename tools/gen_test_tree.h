#pragma once

#include <string>
#include <filesystem>
#include <cstdint>

namespace dupcleaner::tools {

struct TreeConfig {
    std::filesystem::path output_dir;
    size_t num_files = 100000;
    double duplicate_ratio = 0.2; // 20% of files will be duplicates
    size_t min_file_size = 1024; // 1 KB
    size_t max_file_size = 10 * 1024 * 1024; // 10 MB
};

// Generates the synthetic directory tree.
// Returns the total size of all generated files in bytes.
uintmax_t generateTestTree(const TreeConfig& config);

} // namespace dupcleaner::tools
