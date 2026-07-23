#include "scanner.h"
#include <iostream>

namespace fs = std::filesystem;

namespace dupcleaner {

std::vector<FileEntry> DirectoryScanner::scan(const fs::path& root) {
    std::vector<FileEntry> entries;
    skipped_paths_.clear();

    if (!fs::exists(root) || !fs::is_directory(root)) {
        skipped_paths_.push_back("Root is not a valid directory: " + root.string());
        return entries;
    }

    auto options = fs::directory_options::skip_permission_denied;
    std::error_code ec;

    for (auto it = fs::recursive_directory_iterator(root, options, ec); it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) {
            skipped_paths_.push_back("Iteration error: " + ec.message());
            ec.clear();
            continue;
        }

        const auto& entry = *it;
        fs::path p = entry.path();

        // Check for symlinks
        std::error_code symlink_ec;
        if (fs::is_symlink(p, symlink_ec) || symlink_ec) {
            skipped_paths_.push_back("Skipped symlink or inaccessible: " + p.string());
            continue;
        }

        // Check if regular file
        std::error_code is_reg_ec;
        if (!fs::is_regular_file(p, is_reg_ec) || is_reg_ec) {
            std::error_code is_dir_ec;
            if (!fs::is_directory(p, is_dir_ec) && !is_dir_ec) {
                 skipped_paths_.push_back("Skipped non-regular file: " + p.string());
            }
            continue;
        }

        // Get file size
        std::error_code size_ec;
        auto file_size = fs::file_size(p, size_ec);
        if (size_ec) {
            skipped_paths_.push_back("Skipped unreadable size: " + p.string());
            continue;
        }

        // Get last write time
        std::error_code time_ec;
        auto write_time = fs::last_write_time(p, time_ec);
        if (time_ec) {
            skipped_paths_.push_back("Skipped unreadable modified time: " + p.string());
            continue;
        }

        FileEntry file_entry;
        file_entry.path = fs::absolute(p);
        file_entry.size = file_size;
        file_entry.last_modified = write_time;
        entries.push_back(std::move(file_entry));
    }

    return entries;
}

const std::vector<std::string>& DirectoryScanner::getSkippedPaths() const {
    return skipped_paths_;
}

} // namespace dupcleaner
