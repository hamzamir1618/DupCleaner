#include <gtest/gtest.h>
#include "interactive_review.h"
#include <sstream>

using namespace dupcleaner::cli;
using namespace dupcleaner;

class InteractiveReviewTest : public ::testing::Test {
protected:
    std::vector<FileEntry> group;

    void SetUp() override {
        group.push_back({std::filesystem::path("file1.txt"), 100});
        group.push_back({std::filesystem::path("file2.txt"), 200});
        group.push_back({std::filesystem::path("file3.txt"), 300});
    }
};

TEST_F(InteractiveReviewTest, AcceptSuggestion) {
    std::stringstream in("a\n");
    std::stringstream out;
    
    // Suggest keeping index 1 (file2.txt)
    auto decision = resolveGroupInteractively(group, 1, in, out);
    
    EXPECT_FALSE(decision.skip);
    ASSERT_EQ(decision.keep_files.size(), 1);
    EXPECT_EQ(decision.keep_files[0].path, "file2.txt");
    ASSERT_EQ(decision.delete_files.size(), 2);
    EXPECT_EQ(decision.delete_files[0].path, "file1.txt");
    EXPECT_EQ(decision.delete_files[1].path, "file3.txt");
}

TEST_F(InteractiveReviewTest, SkipGroup) {
    std::stringstream in("s\n");
    std::stringstream out;
    
    auto decision = resolveGroupInteractively(group, 0, in, out);
    
    EXPECT_TRUE(decision.skip);
    EXPECT_TRUE(decision.keep_files.empty());
    EXPECT_TRUE(decision.delete_files.empty());
}

TEST_F(InteractiveReviewTest, KeepSpecificFile) {
    std::stringstream in("k3\n");
    std::stringstream out;
    
    auto decision = resolveGroupInteractively(group, 0, in, out);
    
    EXPECT_FALSE(decision.skip);
    ASSERT_EQ(decision.keep_files.size(), 1);
    EXPECT_EQ(decision.keep_files[0].path, "file3.txt");
    ASSERT_EQ(decision.delete_files.size(), 2);
    EXPECT_EQ(decision.delete_files[0].path, "file1.txt");
    EXPECT_EQ(decision.delete_files[1].path, "file2.txt");
}

TEST_F(InteractiveReviewTest, InvalidInputReprompts) {
    // feed garbage, then out of bounds index, then empty line, then 'a'
    std::stringstream in("garbage\nk99\n\na\n");
    std::stringstream out;
    
    auto decision = resolveGroupInteractively(group, 2, in, out);
    
    EXPECT_FALSE(decision.skip);
    ASSERT_EQ(decision.keep_files.size(), 1);
    EXPECT_EQ(decision.keep_files[0].path, "file3.txt");
    
    std::string out_str = out.str();
    EXPECT_NE(out_str.find("Invalid input format."), std::string::npos);
    EXPECT_NE(out_str.find("Invalid file index: 99"), std::string::npos);
}
