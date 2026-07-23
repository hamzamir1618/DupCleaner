#include <gtest/gtest.h>
#include "duplicate_finder.h"

using namespace dupcleaner;

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
    
    // Two files of size 100
    FileEntry e1; e1.path = "1a.txt"; e1.size = 100; entries.push_back(e1);
    FileEntry e2; e2.path = "1b.txt"; e2.size = 100; entries.push_back(e2);
    
    // One file of size 200
    FileEntry e3; e3.path = "2a.txt"; e3.size = 200; entries.push_back(e3);
    
    // Three files of size 300
    FileEntry e4; e4.path = "3a.txt"; e4.size = 300; entries.push_back(e4);
    FileEntry e5; e5.path = "3b.txt"; e5.size = 300; entries.push_back(e5);
    FileEntry e6; e6.path = "3c.txt"; e6.size = 300; entries.push_back(e6);

    auto buckets = DuplicateFinder::groupBySize(entries);
    EXPECT_EQ(buckets.size(), 3);

    DuplicateFinder::filterUniqueSizes(buckets);
    
    // Size 200 should be filtered out
    EXPECT_EQ(buckets.size(), 2);
    
    // Size 100 should have 2 entries
    EXPECT_EQ(buckets[100].size(), 2);
    
    // Size 300 should have 3 entries
    EXPECT_EQ(buckets[300].size(), 3);
}

// NOTE: Known Edge Case
// Zero-byte files all bucket together. Many filesystems have several legitimate 
// empty files (lock files, markers, placeholders) that aren't "duplicates" in a 
// meaningful sense for the user to delete to save space. 
// We currently bucket them together, but this is a known edge case that might need 
// special-casing (e.g., stripping zero-byte buckets entirely) in the future.
TEST(DuplicateFinderTest, ZeroByteFilesBucketTogether) {
    std::vector<FileEntry> entries;
    
    FileEntry e1; e1.path = "empty1.dat"; e1.size = 0; entries.push_back(e1);
    FileEntry e2; e2.path = "empty2.dat"; e2.size = 0; entries.push_back(e2);
    FileEntry e3; e3.path = "empty3.dat"; e3.size = 0; entries.push_back(e3);

    auto buckets = DuplicateFinder::groupBySize(entries);
    DuplicateFinder::filterUniqueSizes(buckets);
    
    // They are grouped as potential duplicates.
    EXPECT_EQ(buckets.size(), 1);
    EXPECT_EQ(buckets[0].size(), 3);
}
