#include <gtest/gtest.h>
#include "CLI/CLI.hpp"
#include "cli_app.h"

using namespace dupcleaner::cli;

TEST(CliArgsTest, MissingPathFails) {
    CLI::App app;
    CliOptions opts;
    setup_cli(app, opts);
    
    EXPECT_THROW(app.parse("scan"), CLI::ParseError);
}

TEST(CliArgsTest, MissingScanSubcommandFails) {
    CLI::App app;
    CliOptions opts;
    setup_cli(app, opts);
    
    EXPECT_THROW(app.parse("/my/path"), CLI::ParseError);
}

TEST(CliArgsTest, MinSizeFiltersCorrectly) {
    CLI::App app;
    CliOptions opts;
    setup_cli(app, opts);
    
    app.parse("scan /my/path --min-size 1024");
    
    EXPECT_TRUE(opts.has_scan_command);
    EXPECT_EQ(opts.path, "/my/path");
    EXPECT_EQ(opts.min_size, 1024);
}

TEST(CliArgsTest, JsonFlagRecognized) {
    CLI::App app;
    CliOptions opts;
    setup_cli(app, opts);
    
    app.parse("scan /my/path --json");
    
    EXPECT_TRUE(opts.has_scan_command);
    EXPECT_EQ(opts.path, "/my/path");
    EXPECT_TRUE(opts.json_output);
    EXPECT_FALSE(opts.verbose);
}

TEST(CliArgsTest, VerboseFlagRecognized) {
    CLI::App app;
    CliOptions opts;
    setup_cli(app, opts);
    
    app.parse("scan /my/path --verbose");
    
    EXPECT_TRUE(opts.has_scan_command);
    EXPECT_TRUE(opts.verbose);
}
