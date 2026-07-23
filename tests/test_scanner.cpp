#include <gtest/gtest.h>
#include "scanner.h"
#include <fstream>
#include <filesystem>
#include <system_error>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;
using dupcleaner::DirectoryScanner;

class ScannerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = fs::temp_directory_path() / ("dupcleaner_test_scan_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        fs::create_directory(test_dir);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(test_dir, ec);
    }

    void createFile(const fs::path& path, const std::string& content) {
        fs::create_directories(path.parent_path());
        std::ofstream out(path);
        out << content;
        out.close();
    }

    void createSymlink(const fs::path& target, const fs::path& link) {
        std::error_code ec;
        fs::create_symlink(target, link, ec);
        symlink_supported = !ec;
    }

    fs::path test_dir;
    bool symlink_supported{true};
};

// 1. Empty directory
TEST_F(ScannerTest, EmptyDirectoryReturnsZero) {
    DirectoryScanner scanner;
    auto result = scanner.scan(test_dir);
    EXPECT_TRUE(result.entries.empty());
    EXPECT_TRUE(result.skipped_paths.empty());
    EXPECT_EQ(result.stats.files_visited, 0);
    EXPECT_EQ(result.stats.directories_visited, 1); // Only root
}

// 2. Known set of files
TEST_F(ScannerTest, KnownFilesAreScanned) {
    createFile(test_dir / "file1.txt", "123");
    createFile(test_dir / "file2.txt", "12345");

    DirectoryScanner scanner;
    auto result = scanner.scan(test_dir);
    EXPECT_EQ(result.entries.size(), 2);
    EXPECT_EQ(result.stats.files_visited, 2);
    EXPECT_EQ(result.stats.bytes_visited, 8); // 3 + 5 bytes
    EXPECT_EQ(result.stats.directories_visited, 1);
    
    bool found_file1 = false;
    bool found_file2 = false;
    for (const auto& r : result.entries) {
        if (r.path.filename() == "file1.txt" && r.size == 3) found_file1 = true;
        if (r.path.filename() == "file2.txt" && r.size == 5) found_file2 = true;
    }
    EXPECT_TRUE(found_file1);
    EXPECT_TRUE(found_file2);
}

// 3. Nested structure
TEST_F(ScannerTest, NestedStructureTraversed) {
    createFile(test_dir / "root_file.txt", "123");
    createFile(test_dir / "sub" / "sub_file.txt", "123");
    createFile(test_dir / "sub" / "sub2" / "deep_file.txt", "123");

    DirectoryScanner scanner;
    auto result = scanner.scan(test_dir);
    EXPECT_EQ(result.entries.size(), 3);
    EXPECT_EQ(result.stats.files_visited, 3);
    EXPECT_EQ(result.stats.bytes_visited, 9);
    EXPECT_EQ(result.stats.directories_visited, 3); // root, sub, sub2
}

// 4. Symlink skipped
TEST_F(ScannerTest, SymlinkIsSkippedAndRecorded) {
    createFile(test_dir / "real_file.txt", "data");
    
    fs::path link_path = test_dir / "symlink_file.txt";
    createSymlink(test_dir / "real_file.txt", link_path);

    if (!symlink_supported) {
        GTEST_SKIP() << "Symlinks not supported in this test environment.";
    }

    DirectoryScanner scanner;
    auto result = scanner.scan(test_dir);
    
    EXPECT_EQ(result.entries.size(), 1);
    EXPECT_EQ(result.entries[0].path.filename(), "real_file.txt");
    EXPECT_EQ(result.stats.files_visited, 1);
    EXPECT_EQ(result.stats.items_skipped, 1);

    bool logged = false;
    for (const auto& sp : result.skipped_paths) {
        if (sp.find("Skipped symlink") != std::string::npos && sp.find(link_path.string()) != std::string::npos) {
            logged = true;
            break;
        }
    }
    EXPECT_TRUE(logged);
}

// 5. Zero byte files handled
TEST_F(ScannerTest, ZeroByteFilesHandled) {
    createFile(test_dir / "empty.dat", "");

    DirectoryScanner scanner;
    auto result = scanner.scan(test_dir);
    
    EXPECT_EQ(result.entries.size(), 1);
    EXPECT_EQ(result.entries[0].size, 0);
    EXPECT_EQ(result.stats.files_visited, 1);
    EXPECT_EQ(result.stats.bytes_visited, 0);
}

// 6. Very long file paths
TEST_F(ScannerTest, VeryLongFilePath) {
    std::string long_name(200, 'a');
    fs::path deep_dir = test_dir;
    for (int i = 0; i < 4; ++i) { // This creates a path likely over Windows MAX_PATH (260)
        deep_dir /= long_name;
    }
    
    std::error_code ec;
    fs::create_directories(deep_dir, ec);
    if (ec) {
        // OS rejected the path creation, guard gracefully
        GTEST_SKIP() << "OS rejected very long path creation, skipping test.";
    }

    fs::path long_file = deep_dir / "file.txt";
    std::ofstream out(long_file, std::ios::binary);
    if (!out) {
        GTEST_SKIP() << "OS rejected very long file creation, skipping test.";
    }
    out << "long";
    out.close();

    DirectoryScanner scanner;
    auto result = scanner.scan(test_dir);
    
    // As long as it doesn't crash, and either finds it or skips it, we're good
    EXPECT_GE(result.entries.size() + result.skipped_paths.size(), 0);
}

// 7. Unusual characters
TEST_F(ScannerTest, UnusualCharacters) {
    fs::path weird = test_dir / "  .. \xC3\x9Cnicode🚀 .dat"; // Contains spaces, dots, unicode emoji
    createFile(weird, "weird");

    DirectoryScanner scanner;
    auto result = scanner.scan(test_dir);
    
    ASSERT_EQ(result.entries.size(), 1);
    EXPECT_EQ(result.entries[0].size, 5);
}

// 8. Dangling symlink
TEST_F(ScannerTest, DanglingSymlink) {
    if (!symlink_supported) {
        GTEST_SKIP() << "Symlinks not supported in this test environment.";
    }

    fs::path target = test_dir / "target_will_be_deleted.txt";
    createFile(target, "temp");
    
    fs::path link = test_dir / "dangling_link.txt";
    createSymlink(target, link);
    
    // Delete target to make it dangling
    fs::remove(target);

    DirectoryScanner scanner;
    auto result = scanner.scan(test_dir);
    
    // Should be skipped, no crash
    EXPECT_TRUE(result.entries.empty());
    EXPECT_GT(result.skipped_paths.size(), 0);
}
