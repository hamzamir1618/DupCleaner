#include <iostream>
#include <filesystem>
#include "CLI/CLI.hpp"
#include "scanner.h"
#include "duplicate_finder.h"
#include "cli_app.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;
using namespace dupcleaner;
using namespace dupcleaner::cli;

int main(int argc, char** argv) {
    CLI::App app{"dupcleaner - Find and manage duplicate files"};
    CliOptions opts;
    setup_cli(app, opts);

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    if (!opts.has_scan_command) {
        return 0;
    }

    fs::path target(opts.path);
    if (!fs::exists(target) || !fs::is_directory(target)) {
        if (opts.json_output) {
            json j;
            j["error"] = "Provided path is not a valid directory: " + opts.path;
            std::cout << j.dump(4) << "\n";
        } else {
            std::cerr << "Error: Provided path is not a valid directory: " << opts.path << "\n";
        }
        return 1;
    }

    if (!opts.json_output && opts.verbose) {
        std::cout << "Scanning directory: " << fs::absolute(target).string() << "...\n";
    }

    DirectoryScanner scanner;
    ScannerOptions scan_opts;
    scan_opts.min_size = opts.min_size;
    
    auto result = scanner.scan(target, scan_opts);
    auto dup_groups = DuplicateFinder::findExactDuplicates(result.entries);

    if (opts.json_output) {
        json j;
        j["scan_stats"] = {
            {"files_visited", result.stats.files_visited},
            {"bytes_visited", result.stats.bytes_visited},
            {"directories_visited", result.stats.directories_visited},
            {"items_skipped", result.stats.items_skipped},
            {"duration_ms", result.stats.duration.count()}
        };
        
        if (opts.verbose) {
            j["skipped_paths"] = result.skipped_paths;
        }

        json groups = json::array();
        uintmax_t total_wasted_space = 0;

        for (const auto& group : dup_groups) {
            uintmax_t file_size = group[0].size;
            uintmax_t group_wasted = file_size * (group.size() - 1);
            total_wasted_space += group_wasted;

            json g;
            g["size_bytes"] = file_size;
            g["wasted_bytes"] = group_wasted;
            json paths = json::array();
            for (const auto& entry : group) {
                paths.push_back(entry.path.string());
            }
            g["files"] = paths;
            groups.push_back(g);
        }

        j["groups"] = groups;
        j["total_wasted_bytes"] = total_wasted_space;

        std::cout << j.dump(2) << "\n";
    } else {
        if (opts.verbose) {
            std::cout << "\n--- Scan Complete ---\n";
            std::cout << "Total files visited: " << result.stats.files_visited << "\n";
            std::cout << "Total bytes visited: " << result.stats.bytes_visited << " bytes\n";
            std::cout << "Directories visited: " << result.stats.directories_visited << "\n";
            std::cout << "Items skipped:       " << result.stats.items_skipped << "\n";
            std::cout << "Scan duration:       " << result.stats.duration.count() << " ms\n";
            
            if (!result.skipped_paths.empty()) {
                std::cout << "\nWarning: " << result.skipped_paths.size() << " items were skipped.\n";
                for (const auto& sp : result.skipped_paths) {
                    std::cout << "  " << sp << "\n";
                }
            }
        }

        if (dup_groups.empty()) {
            std::cout << "No exact duplicates found.\n";
        } else {
            uintmax_t total_wasted_space = 0;
            std::cout << "\nFound " << dup_groups.size() << " exact duplicate groups:\n\n";

            for (size_t i = 0; i < dup_groups.size(); ++i) {
                const auto& group = dup_groups[i];
                uintmax_t file_size = group[0].size;
                uintmax_t group_wasted = file_size * (group.size() - 1);
                total_wasted_space += group_wasted;

                std::cout << "Group " << (i + 1) << " (Size: " << file_size << " bytes, Wasted: " << group_wasted << " bytes):\n";
                for (const auto& entry : group) {
                    std::cout << "  - " << entry.path.string() << "\n";
                }
                std::cout << "\n";
            }
            std::cout << "Total wasted space: " << total_wasted_space << " bytes.\n";
        }
    }

    return 0;
}
