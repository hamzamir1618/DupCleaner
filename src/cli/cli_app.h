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
};

inline void setup_cli(CLI::App& app, CliOptions& opts) {
    app.require_subcommand(1);
    
    auto* scan_cmd = app.add_subcommand("scan", "Scan a directory for exact duplicates");
    scan_cmd->add_option("path", opts.path, "Directory path to scan for duplicates")->required();
    scan_cmd->add_option("--min-size", opts.min_size, "Skip files smaller than this (bytes)");
    scan_cmd->add_flag("--json", opts.json_output, "Output report as JSON");
    scan_cmd->add_flag("--verbose", opts.verbose, "Print skipped paths and detailed stats");
    
    scan_cmd->callback([&opts]() {
        opts.has_scan_command = true;
    });
}

} // namespace cli
} // namespace dupcleaner
