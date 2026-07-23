#include <gtest/gtest.h>
#include "duplicate_finder.h"
#include <fstream>
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;
using namespace dupcleaner;

// --- Bucketing Tests ---

TEST(DuplicateFinderTest, AllUniqueSizesProduceNoCandidates) {
    std::vector<FileEntry> entries;
    for (int i = 1; i <= 5; ++i) {
        FileEntry e;
        e.path = "file" + std::to_string(i) + ".txt";
        e.size = i * 100;
        entries.push_back(e);
    }

    auto buckets = DuplicateFinder::groupBySize(entries);
    EXPECT_EQ(buckets.size(), 5);

    DuplicateFinder::filterUniqueSizes(buckets);
    EXPECT_TRUE(buckets.empty());
}

TEST(DuplicateFinderTest, MatchingSizesGroupedCorrectly) {
    std::vector<FileEntry> entries;
    
    FileEntry e1; e1.path = "1a.txt"; e1.size = 100; entries.push_back(e1);
    FileEntry e2; e2.path = "1b.txt"; e2.size = 100; entries.push_back(e2);
    
    FileEntry e3; e3.path = "2a.txt"; e3.size = 200; entries.push_back(e3);
    
    FileEntry e4; e4.path = "3a.txt"; e4.size = 300; entries.push_back(e4);
    FileEntry e5; e5.path = "3b.txt"; e5.size = 300; entries.push_back(e5);
    FileEntry e6; e6.path = "3c.txt"; e6.size = 300; entries.push_back(e6);

    auto buckets = DuplicateFinder::groupBySize(entries);
    EXPECT_EQ(buckets.size(), 3);

    DuplicateFinder::filterUniqueSizes(buckets);
    
    EXPECT_EQ(buckets.size(), 2);
    EXPECT_EQ(buckets[100].size(), 2);
    EXPECT_EQ(buckets[300].size(), 3);
}

TEST(DuplicateFinderTest, ZeroByteFilesBucketTogether) {
    std::vector<FileEntry> entries;
    
    FileEntry e1; e1.path = "empty1.dat"; e1.size = 0; entries.push_back(e1);
    FileEntry e2; e2.path = "empty2.dat"; e2.size = 0; entries.push_back(e2);
    FileEntry e3; e3.path = "empty3.dat"; e3.size = 0; entries.push_back(e3);

    auto buckets = DuplicateFinder::groupBySize(entries);
    DuplicateFinder::filterUniqueSizes(buckets);
    
    EXPECT_EQ(buckets.size(), 1);
    EXPECT_EQ(buckets[0].size(), 3);
}

// --- IO Identity Tests ---

class DuplicateFinderIOTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = fs::temp_directory_path() / ("dupcleaner_test_dupio_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
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

    void createLargeFile(const fs::path& path, size_t size_bytes, char fill_char, int differing_byte_index = -1, char differ_char = 'X') {
        std::ofstream out(path, std::ios::binary);
        std::vector<char> buffer(64 * 1024, fill_char);
        size_t written = 0;
        while (written < size_bytes) {
            size_t to_write = std::min(buffer.size(), size_bytes - written);
            
            if (differing_byte_index >= 0 && 
                (size_t)differing_byte_index >= written && 
                (size_t)differing_byte_index < written + to_write) {
                
                size_t offset_in_buffer = differing_byte_index - written;
                buffer[offset_in_buffer] = differ_char;
                out.write(buffer.data(), to_write);
                buffer[offset_in_buffer] = fill_char; // restore
            } else {
                out.write(buffer.data(), to_write);
            }
            written += to_write;
        }
        out.close();
    }

    fs::path test_dir;
};

TEST_F(DuplicateFinderIOTest, SelfComparisonReturnsTrue) {
    fs::path p1 = test_dir / "self.dat";
    createFile(p1, "Data");
    EXPECT_TRUE(DuplicateFinder::filesAreIdentical(p1, p1));
}

TEST_F(DuplicateFinderIOTest, LargeIdenticalFilesCompareEqual) {
    fs::path p1 = test_dir / "large1.dat";
    fs::path p2 = test_dir / "large2.dat";
    size_t size = 150 * 1024; // > 64KB chunk
    createLargeFile(p1, size, 'A');
    createLargeFile(p2, size, 'A');
    EXPECT_TRUE(DuplicateFinder::filesAreIdentical(p1, p2));
}

TEST_F(DuplicateFinderIOTest, DifferOnlyInFirstByte) {
    fs::path p1 = test_dir / "first1.dat";
    fs::path p2 = test_dir / "first2.dat";
    size_t size = 150 * 1024;
    createLargeFile(p1, size, 'A');
    createLargeFile(p2, size, 'A', 0, 'B'); // First byte differs
    EXPECT_FALSE(DuplicateFinder::filesAreIdentical(p1, p2));
}

TEST_F(DuplicateFinderIOTest, DifferOnlyInLastByte) {
    fs::path p1 = test_dir / "last1.dat";
    fs::path p2 = test_dir / "last2.dat";
    size_t size = 150 * 1024;
    createLargeFile(p1, size, 'A');
    createLargeFile(p2, size, 'A', size - 1, 'B'); // Last byte differs
    EXPECT_FALSE(DuplicateFinder::filesAreIdentical(p1, p2));
}

// --- End-to-End findExactDuplicates Tests ---

TEST_F(DuplicateFinderIOTest, NoDuplicatesReportEmpty) {
    createFile(test_dir / "1.txt", "A");
    createFile(test_dir / "2.txt", "BB"); // Different size
    createFile(test_dir / "3.txt", "CCC"); // Different size
    
    std::vector<FileEntry> entries;
    for (int i = 1; i <= 3; ++i) {
        FileEntry e;
        e.path = test_dir / (std::to_string(i) + ".txt");
        e.size = fs::file_size(e.path);
        entries.push_back(e);
    }
    
    auto results = DuplicateFinder::findExactDuplicates(entries);
    EXPECT_TRUE(results.empty());
}

TEST_F(DuplicateFinderIOTest, ThreeCopiesAndTwoUnrelated) {
    createFile(test_dir / "dup1.dat", "DATA");
    createFile(test_dir / "dup2.dat", "DATA");
    createFile(test_dir / "dup3.dat", "DATA");
    
    createFile(test_dir / "unrelated1.dat", "NOPE");
    createFile(test_dir / "unrelated2.dat", "12345"); // Diff size
    
    std::vector<FileEntry> entries;
    for (const auto& name : {"dup1.dat", "dup2.dat", "dup3.dat", "unrelated1.dat", "unrelated2.dat"}) {
        FileEntry e;
        e.path = test_dir / name;
        e.size = fs::file_size(e.path);
        entries.push_back(e);
    }
    
    auto results = DuplicateFinder::findExactDuplicates(entries);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].size(), 3);
}

TEST_F(DuplicateFinderIOTest, MultipleDuplicateSets) {
    createFile(test_dir / "set1_a.dat", "FOO");
    createFile(test_dir / "set1_b.dat", "FOO");
    
    createFile(test_dir / "set2_a.dat", "BARBAZ");
    createFile(test_dir / "set2_b.dat", "BARBAZ");
    
    std::vector<FileEntry> entries;
    for (const auto& name : {"set1_a.dat", "set1_b.dat", "set2_a.dat", "set2_b.dat"}) {
        FileEntry e;
        e.path = test_dir / name;
        e.size = fs::file_size(e.path);
        entries.push_back(e);
    }
    
    auto results = DuplicateFinder::findExactDuplicates(entries);
    EXPECT_EQ(results.size(), 2);
}

TEST_F(DuplicateFinderIOTest, FalsePositivesAvoided) {
    // Two files of the same size but different contents
    createFile(test_dir / "false1.dat", "SAME_SIZE_DIFF_A");
    createFile(test_dir / "false2.dat", "SAME_SIZE_DIFF_B");
    
    std::vector<FileEntry> entries;
    for (const auto& name : {"false1.dat", "false2.dat"}) {
        FileEntry e;
        e.path = test_dir / name;
        e.size = fs::file_size(e.path);
        entries.push_back(e);
    }
    
    auto results = DuplicateFinder::findExactDuplicates(entries);
    EXPECT_TRUE(results.empty());
}

TEST_F(DuplicateFinderIOTest, Determinism) {
    std::vector<FileEntry> entries;
    
    // Create 10 groups of 5 pairs of duplicates, meaning 10 sets.
    // They will end up in buckets of the same size if we give them the same size.
    // To ensure a large bucket that triggers thread pooling nondeterminism:
    for (int i = 0; i < 50; ++i) {
        std::string path_a = (test_dir / ("group_a_" + std::to_string(i) + ".txt")).string();
        std::string path_b = (test_dir / ("group_b_" + std::to_string(i) + ".txt")).string();
        std::string content = "identical content " + std::string(2, '0' + (i % 10)); // Always 20 chars
        createFile(path_a, content);
        createFile(path_b, content);
        
        FileEntry ea;
        ea.path = path_a;
        ea.size = 20;
        
        FileEntry eb;
        eb.path = path_b;
        eb.size = 20;
        
        entries.push_back(ea);
        entries.push_back(eb);
    }

    auto first_run = DuplicateFinder::findExactDuplicates(entries);

    // Run 10 times and assert exactly the same order every time
    for (int i = 0; i < 10; ++i) {
        auto subsequent_run = DuplicateFinder::findExactDuplicates(entries);
        ASSERT_EQ(first_run.size(), subsequent_run.size());
        for (size_t j = 0; j < first_run.size(); ++j) {
            ASSERT_EQ(first_run[j].size(), subsequent_run[j].size());
            for (size_t k = 0; k < first_run[j].size(); ++k) {
                EXPECT_EQ(first_run[j][k].path.string(), subsequent_run[j][k].path.string());
            }
        }
    }
}

TEST_F(DuplicateFinderIOTest, FileDeletedBeforeHashingIsGracefullyIgnored) {
    fs::path p1 = test_dir / "keep1.dat";
    fs::path p2 = test_dir / "keep2.dat";
    fs::path deleted_path = test_dir / "deleted.dat";
    
    createFile(p1, "DATA");
    createFile(p2, "DATA");
    createFile(deleted_path, "DATA");
    
    std::vector<FileEntry> entries;
    for (const auto& name : {"keep1.dat", "keep2.dat", "deleted.dat"}) {
        FileEntry e;
        e.path = test_dir / name;
        e.size = fs::file_size(e.path);
        entries.push_back(e);
    }
    
    // Simulate deletion before processing
    fs::remove(deleted_path);
    
    // It should not crash, and should report the remaining duplicates correctly
    auto results = DuplicateFinder::findExactDuplicates(entries);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].size(), 2);
}

// --- Near-Duplicate Clustering Tests ---
#include "stb_image_write.h"

class NearDuplicateFinderTest : public ::testing::Test {
protected:
    fs::path temp_dir;

    void SetUp() override {
        auto now = std::chrono::system_clock::now().time_since_epoch().count();
        temp_dir = fs::temp_directory_path() / ("dupcleaner_neardup_" + std::to_string(now));
        fs::create_directories(temp_dir);
    }

    void TearDown() override {
        if (fs::exists(temp_dir)) {
            std::error_code ec;
            fs::remove_all(temp_dir, ec);
        }
    }

    FileEntry create_png(const std::string& name, int width, int height, bool gradient, bool inverted = false) {
        fs::path p = temp_dir / name;
        std::vector<unsigned char> pixels(width * height * 3);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * 3;
                unsigned char val = gradient ? (x * 255) / width : (((x / 10) % 2 == (y / 10) % 2) ? 255 : 0);
                if (inverted) val = 255 - val;
                pixels[idx] = val;
                pixels[idx+1] = val;
                pixels[idx+2] = val;
            }
        }
        stbi_write_png(p.string().c_str(), width, height, 3, pixels.data(), width * 3);
        
        FileEntry e;
        e.path = p;
        e.size = fs::file_size(p);
        return e;
    }

    FileEntry create_invalid(const std::string& name) {
        fs::path p = temp_dir / name;
        std::ofstream ofs(p);
        ofs << "Not an image!";
        ofs.close();
        FileEntry e;
        e.path = p;
        e.size = fs::file_size(p);
        return e;
    }
};

TEST_F(NearDuplicateFinderTest, IdenticalImagesCluster) {
    std::vector<FileEntry> entries;
    entries.push_back(create_png("img1.jpg", 100, 100, true));
    entries.push_back(create_png("img2.jpg", 100, 100, true));
    
    auto result = DuplicateFinder::findNearDuplicateImages(entries, 5);
    ASSERT_EQ(result.groups.size(), 1);
    EXPECT_EQ(result.groups[0].members.size(), 2);
    EXPECT_TRUE(result.skipped_paths.empty());
}

TEST_F(NearDuplicateFinderTest, SimilarImagesCluster) {
    std::vector<FileEntry> entries;
    entries.push_back(create_png("img1.jpg", 100, 100, true));
    entries.push_back(create_png("img2.jpg", 200, 150, true)); // Resized
    
    auto result = DuplicateFinder::findNearDuplicateImages(entries, 5);
    ASSERT_EQ(result.groups.size(), 1);
    EXPECT_EQ(result.groups[0].members.size(), 2);
}

TEST_F(NearDuplicateFinderTest, DifferentImagesDoNotCluster) {
    std::vector<FileEntry> entries;
    entries.push_back(create_png("img1.jpg", 100, 100, true, false));
    entries.push_back(create_png("img2.jpg", 100, 100, true, true)); // Inverted
    
    auto result = DuplicateFinder::findNearDuplicateImages(entries, 5);
    EXPECT_TRUE(result.groups.empty());
}

TEST_F(NearDuplicateFinderTest, InvalidImagesSkipped) {
    std::vector<FileEntry> entries;
    entries.push_back(create_png("img1.jpg", 100, 100, true));
    entries.push_back(create_invalid("bad.jpg")); // Corrupt / fake image
    entries.push_back(create_png("img2.png", 100, 100, true));
    
    auto result = DuplicateFinder::findNearDuplicateImages(entries, 5);
    ASSERT_EQ(result.groups.size(), 1); // img1 and img2 cluster
    ASSERT_EQ(result.skipped_paths.size(), 1);
    EXPECT_EQ(result.skipped_paths[0].filename().string(), "bad.jpg");
}
