#include <gtest/gtest.h>
#include "hasher.h"
#include "dupcleaner/exceptions.h"
#include <fstream>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;
using dupcleaner::FileHasher;
using dupcleaner::DupCleanerIOException;

class HasherTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = fs::temp_directory_path() / ("dupcleaner_test_hasher_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        fs::create_directory(test_dir);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(test_dir, ec);
    }

    void createFile(const fs::path& path, const std::string& content) {
        std::ofstream out(path, std::ios::binary);
        out << content;
        out.close();
    }

    void createLargeFile(const fs::path& path, size_t size_bytes, char fill_char) {
        std::ofstream out(path, std::ios::binary);
        std::vector<char> buffer(64 * 1024, fill_char);
        size_t written = 0;
        while (written < size_bytes) {
            size_t to_write = std::min(buffer.size(), size_bytes - written);
            out.write(buffer.data(), to_write);
            written += to_write;
        }
        out.close();
    }

    fs::path test_dir;
};

TEST_F(HasherTest, IdenticalContentsProduceIdenticalFingerprints) {
    fs::path p1 = test_dir / "id1.dat";
    fs::path p2 = test_dir / "id2.dat";
    
    std::string content = "Testing identical content hashing";
    createFile(p1, content);
    createFile(p2, content);

    uint64_t h1 = FileHasher::fingerprint(p1);
    uint64_t h2 = FileHasher::fingerprint(p2);
    
    EXPECT_EQ(h1, h2);
}

TEST_F(HasherTest, DifferentContentsProduceDifferentFingerprints) {
    fs::path p1 = test_dir / "diff1.dat";
    fs::path p2 = test_dir / "diff2.dat";
    fs::path p3 = test_dir / "diff3.dat";
    
    createFile(p1, "Pattern A");
    createFile(p2, "Pattern B");
    createFile(p3, "pattern A"); // case difference

    uint64_t h1 = FileHasher::fingerprint(p1);
    uint64_t h2 = FileHasher::fingerprint(p2);
    uint64_t h3 = FileHasher::fingerprint(p3);
    
    EXPECT_NE(h1, h2);
    EXPECT_NE(h1, h3);
    EXPECT_NE(h2, h3);
}

TEST_F(HasherTest, EmptyFileHashesProperly) {
    fs::path p = test_dir / "empty.dat";
    createFile(p, "");

    // xxHash of empty string is a valid non-zero hash
    EXPECT_NO_THROW({
        uint64_t h = FileHasher::fingerprint(p);
        EXPECT_NE(h, 0); 
    });
}

TEST_F(HasherTest, LargeFileChunkingWorks) {
    fs::path p1 = test_dir / "large1.dat";
    fs::path p2 = test_dir / "large2.dat";
    
    // Create files larger than 64KB (e.g. 150KB) to exercise the chunking while-loop
    size_t large_size = 150 * 1024;
    createLargeFile(p1, large_size, 'A');
    createLargeFile(p2, large_size, 'B');

    uint64_t h1 = FileHasher::fingerprint(p1);
    uint64_t h2 = FileHasher::fingerprint(p2);
    
    EXPECT_NE(h1, h2);
    
    fs::path p1_copy = test_dir / "large1_copy.dat";
    createLargeFile(p1_copy, large_size, 'A');
    EXPECT_EQ(h1, FileHasher::fingerprint(p1_copy));
}

TEST_F(HasherTest, NonexistentFileThrowsException) {
    fs::path missing = test_dir / "does_not_exist.dat";
    
    EXPECT_THROW({
        FileHasher::fingerprint(missing);
    }, DupCleanerIOException);
}
