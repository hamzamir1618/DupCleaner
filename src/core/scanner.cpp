#include "scanner.h"
#include <iostream>

namespace fs = std::filesystem;

namespace dupcleaner {

ScanResult DirectoryScanner::scan(const fs::path& root, const ScannerOptions& scan_opts) {
    ScanResult result;
    auto start_time = std::chrono::high_resolution_clock::now();

    if (!fs::exists(root) || !fs::is_directory(root)) {
        result.skipped_paths.push_back("Root is not a valid directory: " + root.string());
        result.stats.items_skipped++;
        auto end_time = std::chrono::high_resolution_clock::now();
        result.stats.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        return result;
    }

    auto options = fs::directory_options::skip_permission_denied;
    std::error_code ec;

    // Root counts as 1 directory
    result.stats.directories_visited++;

    for (auto it = fs::recursive_directory_iterator(root, options, ec); it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) {
            result.skipped_paths.push_back("Iteration error: " + ec.message());
            result.stats.items_skipped++;
            ec.clear();
            continue;
        }

        const auto& entry = *it;
        fs::path p = entry.path();

        // Check if directory
        std::error_code is_dir_ec;
        if (fs::is_directory(p, is_dir_ec)) {
            result.stats.directories_visited++;
            continue;
        } else if (is_dir_ec) {
            result.skipped_paths.push_back("Skipped unreadable item: " + p.string());
            result.stats.items_skipped++;
            continue;
        }

        // Check for symlinks
        std::error_code symlink_ec;
        if (fs::is_symlink(p, symlink_ec) || symlink_ec) {
            result.skipped_paths.push_back("Skipped symlink or inaccessible: " + p.string());
            result.stats.items_skipped++;
            continue;
        }

        // Check if regular file
        std::error_code is_reg_ec;
        if (!fs::is_regular_file(p, is_reg_ec) || is_reg_ec) {
            result.skipped_paths.push_back("Skipped non-regular file: " + p.string());
            result.stats.items_skipped++;
            continue;
        }

        // Get file size
        std::error_code size_ec;
        auto file_size = fs::file_size(p, size_ec);
        if (size_ec) {
            result.skipped_paths.push_back("Skipped unreadable size: " + p.string());
            result.stats.items_skipped++;
            continue;
        }

        if (file_size < scan_opts.min_size) {
            continue; // Skip without reporting as an error
        }

        // Get last write time
        std::error_code time_ec;
        auto write_time = fs::last_write_time(p, time_ec);
        if (time_ec) {
            result.skipped_paths.push_back("Skipped unreadable modified time: " + p.string());
            result.stats.items_skipped++;
            continue;
        }

        FileEntry file_entry;
        file_entry.path = fs::absolute(p);
        file_entry.size = file_size;
        file_entry.last_modified = write_time;
        
        result.entries.push_back(std::move(file_entry));
        result.stats.files_visited++;
        result.stats.bytes_visited += file_size;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    result.stats.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    return result;
}

} // namespace dupcleaner
