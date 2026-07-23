#include <gtest/gtest.h>
#include "CLI/CLI.hpp"
#include "cli_app.h"

using namespace dupcleaner::cli;

TEST(CliArgsTest, MissingPathFails) {
    CLI::App app;
    CliOptions opts;
    setup_cli(app, opts);
    
    const char* argv[] = {"dupcleaner", "scan"};
    EXPECT_THROW(app.parse(sizeof(argv)/sizeof(argv[0]), argv), CLI::ParseError);
}

TEST(CliArgsTest, MissingScanSubcommandFails) {
    CLI::App app;
    CliOptions opts;
    setup_cli(app, opts);
    
    const char* argv[] = {"dupcleaner", "my_path"};
    EXPECT_THROW(app.parse(sizeof(argv)/sizeof(argv[0]), argv), CLI::ParseError);
}

TEST(CliArgsTest, MinSizeFiltersCorrectly) {
    CLI::App app;
    CliOptions opts;
    setup_cli(app, opts);
    
    const char* argv[] = {"dupcleaner", "scan", "my_path", "--min-size", "1024"};
    app.parse(sizeof(argv)/sizeof(argv[0]), argv);
    
    EXPECT_TRUE(opts.has_scan_command);
    EXPECT_EQ(opts.path, "my_path");
    EXPECT_EQ(opts.min_size, 1024);
}

TEST(CliArgsTest, JsonFlagRecognized) {
    CLI::App app;
    CliOptions opts;
    setup_cli(app, opts);
    
    const char* argv[] = {"dupcleaner", "scan", "my_path", "--json"};
    app.parse(sizeof(argv)/sizeof(argv[0]), argv);
    
    EXPECT_TRUE(opts.has_scan_command);
    EXPECT_EQ(opts.path, "my_path");
    EXPECT_TRUE(opts.json_output);
    EXPECT_FALSE(opts.verbose);
}

TEST(CliArgsTest, VerboseFlagRecognized) {
    CLI::App app;
    CliOptions opts;
    setup_cli(app, opts);
    
    const char* argv[] = {"dupcleaner", "scan", "my_path", "--verbose"};
    app.parse(sizeof(argv)/sizeof(argv[0]), argv);
    
    EXPECT_TRUE(opts.has_scan_command);
    EXPECT_TRUE(opts.verbose);
}
