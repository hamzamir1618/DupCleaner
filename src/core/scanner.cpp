#include "scanner.h"
#include <iostream>
#include <regex>

namespace fs = std::filesystem;

namespace dupcleaner {

std::string globToRegex(const std::string& glob) {
    std::string regex = "^";
    for (char c : glob) {
        switch (c) {
            case '*': regex += ".*"; break;
            case '?': regex += "."; break;
            case '.': case '+': case '(': case ')': case '[': case ']': 
            case '{': case '}': case '^': case '$': case '|': case '\\': 
                regex += "\\"; regex += c; break;
            default: regex += c; break;
        }
    }
    regex += "$";
    return regex;
}

ScanResult DirectoryScanner::scan(const fs::path& root, const ScannerOptions& scan_opts, std::atomic<bool>* cancelFlag, const ProgressTracker& progress) {
    ScanResult result;
    auto start_time = std::chrono::high_resolution_clock::now();

    if (!fs::exists(root) || !fs::is_directory(root)) {
        result.skipped_paths.push_back("Root is not a valid directory: " + root.u8string());
        result.stats.items_skipped++;
        auto end_time = std::chrono::high_resolution_clock::now();
        result.stats.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        return result;
    }

    auto options = fs::directory_options::skip_permission_denied;
    std::error_code ec;

    // Root counts as 1 directory
    result.stats.directories_visited++;

    // Prepare exclusion regexes
    std::vector<std::regex> exclude_regexes;
    auto add_excludes = [&](const std::vector<std::string>& patterns) {
        for (const auto& pattern : patterns) {
            std::string converted = pattern;
            std::replace(converted.begin(), converted.end(), '\\', '/'); // normalize backslashes for generic_string matching
            auto flag = std::regex_constants::ECMAScript;
#ifdef _WIN32
            flag |= std::regex_constants::icase;
#endif
            exclude_regexes.emplace_back(globToRegex(converted), flag);
        }
    };

    if (!scan_opts.no_default_excludes) {
        std::vector<std::string> default_excludes;
#ifdef _WIN32
        default_excludes = {"*/Windows/WinSxS*", "*/Windows/System32*", "*/$Recycle.Bin*", "*/System Volume Information*"};
#elif defined(__APPLE__)
        default_excludes = {"*/System*", "*/private/var/db*"};
#else
        default_excludes = {"*/proc*", "*/sys*", "*/dev*"};
#endif
        add_excludes(default_excludes);
    }
    if (!scan_opts.exclude_patterns.empty()) {
        add_excludes(scan_opts.exclude_patterns);
    }

    for (auto it = fs::recursive_directory_iterator(root, options, ec); it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (cancelFlag && cancelFlag->load(std::memory_order_relaxed)) {
            result.skipped_paths.push_back("Scan cancelled by user.");
            break;
        }

        if (ec) {
            result.skipped_paths.push_back("Iteration error: " + ec.message());
            result.stats.items_skipped++;
            ec.clear();
            continue;
        }

        const auto& entry = *it;
        const fs::path& p = entry.path();

        // Check exclusions
        bool excluded = false;
        std::string generic_p = p.generic_u8string();
        for (const auto& rx : exclude_regexes) {
            if (std::regex_search(generic_p, rx)) {
                excluded = true;
                break;
            }
        }

        if (excluded) {
            std::error_code dir_ec;
            if (entry.is_directory(dir_ec)) {
                it.disable_recursion_pending();
                result.skipped_paths.push_back("Excluded by pattern: " + p.u8string() + " (system directory / excluded)");
            } else {
                result.skipped_paths.push_back("Excluded by pattern: " + p.u8string());
            }
            result.stats.items_skipped++;
            continue;
        }

        // Check if directory
        std::error_code is_dir_ec;
        if (entry.is_directory(is_dir_ec)) {
            result.stats.directories_visited++;
            continue;
        } else if (is_dir_ec) {
            result.skipped_paths.push_back("Skipped unreadable item: " + p.u8string());
            result.stats.items_skipped++;
            continue;
        }

        // Check for symlinks
        std::error_code symlink_ec;
        if (entry.is_symlink(symlink_ec) || symlink_ec) {
            result.skipped_paths.push_back("Skipped symlink or inaccessible: " + p.u8string());
            result.stats.items_skipped++;
            continue;
        }

        // Check if regular file
        std::error_code is_reg_ec;
        if (!entry.is_regular_file(is_reg_ec) || is_reg_ec) {
            result.skipped_paths.push_back("Skipped non-regular file: " + p.u8string());
            result.stats.items_skipped++;
            continue;
        }

        // Get file size
        std::error_code size_ec;
        auto file_size = entry.file_size(size_ec);
        if (size_ec) {
            result.skipped_paths.push_back("Skipped unreadable size: " + p.u8string());
            result.stats.items_skipped++;
            continue;
        }

        if (file_size < scan_opts.min_size) {
            continue; // Skip without reporting as an error
        }

        // Get last write time
        std::error_code time_ec;
        auto write_time = entry.last_write_time(time_ec);
        if (time_ec) {
            result.skipped_paths.push_back("Skipped unreadable modified time: " + p.u8string());
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
        
        if (progress.files_visited) {
            progress.files_visited->store(result.stats.files_visited, std::memory_order_relaxed);
        }
        if (progress.bytes_visited) {
            progress.bytes_visited->store(result.stats.bytes_visited, std::memory_order_relaxed);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    result.stats.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    return result;
}

} // namespace dupcleaner
