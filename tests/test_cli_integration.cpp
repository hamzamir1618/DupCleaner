#define _CRT_SECURE_NO_WARNINGS
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <memory>
#include <array>
#include <cstdlib>
#include "nlohmann/json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

class CliIntegrationTest : public ::testing::Test {
protected:
    fs::path temp_dir;

    void SetUp() override {
        // Create a unique temp directory
        std::string dirname = "dupcleaner_cli_test_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        temp_dir = fs::temp_directory_path() / dirname;
        fs::create_directory(temp_dir);

        // Create 2 duplicates and 1 unique file
        create_file("dup1.txt", "identical content");
        create_file("dup2.txt", "identical content");
        create_file("unique.txt", "unique content!");
    }

    void TearDown() override {
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
    }

    void create_file(const std::string& name, const std::string& content) {
        std::ofstream ofs(temp_dir / name);
        ofs << content;
    }

    std::string run_cli(const std::string& subcmd, const std::string& extra_args, const std::string& stdin_input = "") {
        const char* env_exe = std::getenv("DUPCLEANER_CLI_EXE");
        if (!env_exe) {
            throw std::runtime_error("DUPCLEANER_CLI_EXE environment variable not set");
        }
        std::string exe_path = env_exe;
        std::string cmd;

        if (!stdin_input.empty()) {
            fs::path in_file = temp_dir / "stdin.txt";
            std::ofstream ofs(in_file);
            ofs << stdin_input;
            ofs.close();

#ifdef _WIN32
            cmd = "\"\"" + exe_path + "\" " + subcmd + " \"" + temp_dir.string() + "\" " + extra_args + " < \"" + in_file.string() + "\"\"";
#else
            cmd = "\"" + exe_path + "\" " + subcmd + " \"" + temp_dir.string() + "\" " + extra_args + " < \"" + in_file.string() + "\"";
#endif
        } else {
#ifdef _WIN32
            cmd = "\"\"" + exe_path + "\" " + subcmd + " \"" + temp_dir.string() + "\" " + extra_args + "\"";
#else
            cmd = "\"" + exe_path + "\" " + subcmd + " \"" + temp_dir.string() + "\" " + extra_args;
#endif
        }

        std::array<char, 128> buffer;
        std::string result;
#ifdef _WIN32
        std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
#else
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
#endif
        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }
};

TEST_F(CliIntegrationTest, ScanHumanReadableOutput) {
    std::string output = run_cli("scan", "");
    EXPECT_NE(output.find("Found 1 exact duplicate groups:"), std::string::npos);
    EXPECT_NE(output.find("Total wasted space: 17 bytes."), std::string::npos);
}

TEST_F(CliIntegrationTest, ScanJsonOutput) {
    std::string output = run_cli("scan", "--json");
    json j;
    ASSERT_NO_THROW({ j = json::parse(output); }) << "CLI output was not valid JSON:\n" << output;
    ASSERT_TRUE(j.contains("total_wasted_bytes"));
    EXPECT_EQ(j["total_wasted_bytes"], 17);
}

TEST_F(CliIntegrationTest, CleanDryRunPreventsDeletion) {
    std::string output = run_cli("clean", "--dry-run --yes");
    EXPECT_NE(output.find("[Dry-run] Exiting without making changes."), std::string::npos);
    
    EXPECT_TRUE(fs::exists(temp_dir / "dup1.txt"));
    EXPECT_TRUE(fs::exists(temp_dir / "dup2.txt"));
}

TEST_F(CliIntegrationTest, CleanInteractivePiping) {
    // Pipe "y\n" to simulate user confirming the prompt
    std::string output = run_cli("clean", "--trash", "y\n");
    EXPECT_NE(output.find("Proceed with deletion?"), std::string::npos);
    EXPECT_NE(output.find("Successfully moved files to trash."), std::string::npos);

    // One of the dups should be deleted (moved to trash)
    bool dup1 = fs::exists(temp_dir / "dup1.txt");
    bool dup2 = fs::exists(temp_dir / "dup2.txt");
    EXPECT_TRUE(dup1 != dup2); // Only one survives
    EXPECT_TRUE(fs::exists(temp_dir / ".dupcleaner_trash"));
}

TEST_F(CliIntegrationTest, CleanYesAndUndo) {
    std::string output = run_cli("clean", "--yes --trash");
    EXPECT_NE(output.find("Successfully moved files to trash."), std::string::npos);

    bool dup1 = fs::exists(temp_dir / "dup1.txt");
    bool dup2 = fs::exists(temp_dir / "dup2.txt");
    EXPECT_TRUE(dup1 != dup2); // Only one survives

    std::string undo_output = run_cli("undo", "");
    EXPECT_NE(undo_output.find("Successfully restored the most recent deletion batch."), std::string::npos);

    // Both should be back
    EXPECT_TRUE(fs::exists(temp_dir / "dup1.txt"));
    EXPECT_TRUE(fs::exists(temp_dir / "dup2.txt"));
}
