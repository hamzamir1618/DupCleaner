#pragma once
#include "CLI/CLI.hpp"
#include <string>
#include <cstdint>

namespace dupcleaner {
namespace cli {

struct CliOptions {
    std::string path;
    uintmax_t min_size{0};
    bool json_output{false};
    bool verbose{false};
    
    bool has_scan_command{false};
    bool has_clean_command{false};
    bool has_undo_command{false};
    
    std::string strategy{"oldest"};
    bool permanent{false};
    bool trash{true};
    bool dry_run{false};
    bool yes{false};
    bool interactive{false};

    bool include_near_duplicates{false};
    int similarity_threshold{10};
};

inline void setup_cli(CLI::App& app, CliOptions& opts) {
    app.require_subcommand(1);
    
    // --- SCAN SUBCOMMAND ---
    auto* scan_cmd = app.add_subcommand("scan", "Scan a directory for exact duplicates");
    scan_cmd->add_option("path", opts.path, "Directory path to scan for duplicates")->required();
    scan_cmd->add_option("--min-size", opts.min_size, "Skip files smaller than this (bytes)");
    scan_cmd->add_flag("--json", opts.json_output, "Output report as JSON");
    scan_cmd->add_flag("--verbose", opts.verbose, "Print skipped paths and detailed stats");
    scan_cmd->add_flag("--include-near-duplicates", opts.include_near_duplicates, "Also scan for near-duplicate images using perceptual hashing");
    scan_cmd->add_option("--similarity-threshold", opts.similarity_threshold, "Hamming distance threshold for near-duplicates (default: 10)");
    scan_cmd->callback([&opts]() { opts.has_scan_command = true; });

    // --- CLEAN SUBCOMMAND ---
    auto* clean_cmd = app.add_subcommand("clean", "Safely delete exact duplicates");
    clean_cmd->add_option("path", opts.path, "Directory path to scan and clean")->required();
    clean_cmd->add_option("--min-size", opts.min_size, "Skip files smaller than this (bytes)");
    clean_cmd->add_option("--strategy", opts.strategy, "Deletion strategy: oldest, newest, alpha-first")
        ->check(CLI::IsMember({"oldest", "newest", "alpha-first"}));
    
    auto* trash_flag = clean_cmd->add_flag("--trash", opts.trash, "Move files to .dupcleaner_trash (default)");
    auto* perm_flag = clean_cmd->add_flag("--permanent", opts.permanent, "Permanently delete files (irreversible!)");
    trash_flag->excludes(perm_flag);
    perm_flag->excludes(trash_flag);

    clean_cmd->add_flag("--dry-run", opts.dry_run, "Print deletion plan without modifying filesystem");
    clean_cmd->add_flag("--yes", opts.yes, "Skip interactive confirmation prompt");
    clean_cmd->add_flag("--interactive", opts.interactive, "Interactively review each duplicate group");
    clean_cmd->add_flag("--include-near-duplicates", opts.include_near_duplicates, "Also clean near-duplicate images using perceptual hashing");
    clean_cmd->add_option("--similarity-threshold", opts.similarity_threshold, "Hamming distance threshold for near-duplicates (default: 10)");
    clean_cmd->callback([&opts]() { 
        if (opts.yes && opts.interactive) {
            throw CLI::ValidationError("--yes and --interactive are mutually exclusive.");
        }
        opts.has_clean_command = true; 
    });

    // --- UNDO SUBCOMMAND ---
    auto* undo_cmd = app.add_subcommand("undo", "Undo the last trash batch");
    undo_cmd->add_option("path", opts.path, "Directory path where the trash resides")->required();
    undo_cmd->callback([&opts]() { opts.has_undo_command = true; });
}

} // namespace cli
} // namespace dupcleaner
