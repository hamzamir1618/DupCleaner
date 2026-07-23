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

    std::string run_cli(bool use_json) {
        const char* env_exe = std::getenv("DUPCLEANER_CLI_EXE");
        if (!env_exe) {
            throw std::runtime_error("DUPCLEANER_CLI_EXE environment variable not set");
        }
        std::string exe_path = env_exe;
#ifdef _WIN32
        // Windows cmd.exe /c parsing is weird. 
        // Robust format: ""path\to\exe" scan "path\to\dir""
        std::string cmd = "\"\"" + exe_path + "\" scan \"" + temp_dir.string() + "\"";
        if (use_json) cmd += " --json";
        cmd += "\"";
#else
        std::string cmd = "\"" + exe_path + "\" scan \"" + temp_dir.string() + "\"";
        if (use_json) cmd += " --json";
#endif

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

TEST_F(CliIntegrationTest, HumanReadableOutput) {
    std::string output = run_cli(false);
    
    // Check for human readable strings
    EXPECT_NE(output.find("Found 1 exact duplicate groups:"), std::string::npos);
    EXPECT_NE(output.find("Total wasted space: 17 bytes."), std::string::npos);
}

TEST_F(CliIntegrationTest, JsonOutput) {
    std::string output = run_cli(true);
    
    // Parse JSON
    json j;
    ASSERT_NO_THROW({
        j = json::parse(output);
    }) << "CLI output was not valid JSON:\n" << output;
    
    ASSERT_TRUE(j.contains("total_wasted_bytes"));
    EXPECT_EQ(j["total_wasted_bytes"], 17);
    
    ASSERT_TRUE(j.contains("groups"));
    EXPECT_EQ(j["groups"].size(), 1);
    
    EXPECT_EQ(j["groups"][0]["size_bytes"], 17);
    EXPECT_EQ(j["groups"][0]["wasted_bytes"], 17);
    EXPECT_EQ(j["groups"][0]["files"].size(), 2);
}
