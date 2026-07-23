#include "gen_test_tree.h"
#include <iostream>
#include <fstream>
#include <random>
#include <vector>

namespace dupcleaner::tools {

uintmax_t generateTestTree(const TreeConfig& config) {
    if (std::filesystem::exists(config.output_dir)) {
        std::filesystem::remove_all(config.output_dir);
    }
    std::filesystem::create_directories(config.output_dir);

    std::mt19937 gen(42); // Deterministic seed
    std::uniform_real_distribution<> dup_dist(0.0, 1.0);
    std::uniform_int_distribution<size_t> size_dist(config.min_file_size, config.max_file_size);
    std::uniform_int_distribution<int> dir_dist(1, 100); // 100 subdirectories

    size_t num_duplicates = static_cast<size_t>(config.num_files * config.duplicate_ratio);
    size_t num_unique = config.num_files - num_duplicates;

    std::vector<std::string> unique_contents;
    unique_contents.reserve(num_unique);

    uintmax_t total_bytes = 0;

    // Pre-generate unique files
    for (size_t i = 0; i < num_unique; ++i) {
        std::filesystem::path dir = config.output_dir / ("dir_" + std::to_string(dir_dist(gen)));
        std::filesystem::create_directories(dir);

        std::filesystem::path filepath = dir / ("unique_" + std::to_string(i) + ".dat");

        size_t size = size_dist(gen);
        std::string content;
        content.reserve(size);
        // Fill with some pattern based on index to ensure uniqueness
        for (size_t j = 0; j < size; ++j) {
            content.push_back(static_cast<char>((i + j) % 256));
        }

        std::ofstream out(filepath, std::ios::binary);
        out.write(content.data(), content.size());
        
        unique_contents.push_back(std::move(content));
        total_bytes += size;
    }

    if (num_unique == 0) return 0; // Edge case
    std::uniform_int_distribution<size_t> unique_index_dist(0, num_unique - 1);

    // Generate duplicates
    for (size_t i = 0; i < num_duplicates; ++i) {
        std::filesystem::path dir = config.output_dir / ("dir_" + std::to_string(dir_dist(gen)));
        std::filesystem::create_directories(dir);

        std::filesystem::path filepath = dir / ("dup_" + std::to_string(i) + ".dat");

        size_t src_idx = unique_index_dist(gen);
        const std::string& content = unique_contents[src_idx];

        std::ofstream out(filepath, std::ios::binary);
        out.write(content.data(), content.size());

        total_bytes += content.size();
    }

    return total_bytes;
}

} // namespace dupcleaner::tools

// Standalone CLI logic for the tool
#ifndef DUPCLEANER_TESTING_ENV
int main(int argc, char** argv) {
    dupcleaner::tools::TreeConfig config;
    if (argc > 1) config.output_dir = argv[1];
    else config.output_dir = "synthetic_tree";

    std::cout << "Generating tree with " << config.num_files << " files...\n";
    uintmax_t size = dupcleaner::tools::generateTestTree(config);
    std::cout << "Done! Generated " << size / (1024 * 1024) << " MB.\n";
    return 0;
}
#endif
