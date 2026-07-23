#include <iostream>
#include <filesystem>
#include "CLI/CLI.hpp"
#include "scanner.h"

namespace fs = std::filesystem;
using dupcleaner::DirectoryScanner;

int main(int argc, char** argv) {
    CLI::App app{"dupcleaner - Find and manage duplicate files"};

    std::string scan_path;
    app.add_option("--path,-p", scan_path, "Directory path to scan for duplicates")->required();

    CLI11_PARSE(app, argc, argv);

    fs::path target(scan_path);
    if (!fs::exists(target) || !fs::is_directory(target)) {
        std::cerr << "Error: Provided path is not a valid directory: " << scan_path << "\n";
        return 1;
    }

    std::cout << "Scanning directory: " << fs::absolute(target).string() << "...\n";

    DirectoryScanner scanner;
    auto result = scanner.scan(target);

    std::cout << "\n--- Scan Complete ---\n";
    std::cout << "Total files visited: " << result.stats.files_visited << "\n";
    std::cout << "Total bytes visited: " << result.stats.bytes_visited << " bytes\n";
    std::cout << "Directories visited: " << result.stats.directories_visited << "\n";
    std::cout << "Items skipped:       " << result.stats.items_skipped << "\n";
    std::cout << "Scan duration:       " << result.stats.duration.count() << " ms\n";

    if (!result.skipped_paths.empty()) {
        std::cout << "\nWarning: " << result.skipped_paths.size() << " items were skipped (symlinks or unreadable).\n";
    }

    return 0;
}
