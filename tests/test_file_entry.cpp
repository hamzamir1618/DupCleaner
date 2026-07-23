#include <gtest/gtest.h>
#include "dupcleaner/file_entry.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;
using dupcleaner::FileEntry;

class FileEntryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary files for testing
        test_file_path_1 = fs::temp_directory_path() / "dupcleaner_test_file_1.tmp";
        test_file_path_2 = fs::temp_directory_path() / "dupcleaner_test_file_2.tmp";
        
        std::ofstream out1(test_file_path_1);
        out1 << "Hello dupcleaner!";
        out1.close();

        std::ofstream out2(test_file_path_2);
        out2 << "Different content for file 2";
        out2.close();
    }

    void TearDown() override {
        // Clean up
        fs::remove(test_file_path_1);
        fs::remove(test_file_path_2);
    }

    fs::path test_file_path_1;
    fs::path test_file_path_2;
};

TEST_F(FileEntryTest, ConstructsCorrectlyFromRealFile) {
    FileEntry entry;
    entry.path = fs::absolute(test_file_path_1);
    entry.size = fs::file_size(test_file_path_1);
    entry.last_modified = fs::last_write_time(test_file_path_1);

    EXPECT_EQ(entry.path, fs::absolute(test_file_path_1));
    EXPECT_EQ(entry.size, 17); // "Hello dupcleaner!" is 17 bytes
    EXPECT_FALSE(entry.fast_hash_fingerprint.has_value());
}

TEST_F(FileEntryTest, EqualityComparison) {
    FileEntry entry1;
    entry1.path = fs::absolute(test_file_path_1);
    entry1.size = fs::file_size(test_file_path_1);
    entry1.last_modified = fs::last_write_time(test_file_path_1);
    
    FileEntry entry1_copy = entry1; // Copy via default copy constructor

    FileEntry entry2;
    entry2.path = fs::absolute(test_file_path_2);
    entry2.size = fs::file_size(test_file_path_2);
    entry2.last_modified = fs::last_write_time(test_file_path_2);

    EXPECT_EQ(entry1, entry1_copy);
    EXPECT_NE(entry1, entry2);
}
