#include <gtest/gtest.h>
#include "formatting.h"
#include "dupcleaner/file_entry.h"

using namespace dupcleaner;
using namespace dupcleaner::gui;

TEST(GuiFormattingTest, FormatBytes) {
    EXPECT_EQ(formatBytes(0), "0 B");
    EXPECT_EQ(formatBytes(1023), "1023 B");
    EXPECT_EQ(formatBytes(1024), "1.00 KB");
    EXPECT_EQ(formatBytes(1536), "1.50 KB");
    EXPECT_EQ(formatBytes(1048576), "1.00 MB");
    EXPECT_EQ(formatBytes(1048576 + 524288), "1.50 MB");
    EXPECT_EQ(formatBytes(1073741824ull), "1.00 GB");
    EXPECT_EQ(formatBytes(1099511627776ull), "1.00 TB");
}

TEST(GuiFormattingTest, CalculateWastedSpace) {
    std::vector<FileEntry> empty_group;
    EXPECT_EQ(calculateWastedSpace(empty_group), 0);

    std::vector<FileEntry> single_group = { {"file1", 1000} };
    EXPECT_EQ(calculateWastedSpace(single_group), 0);

    std::vector<FileEntry> group_of_two = { {"file1", 1000}, {"file2", 1000} };
    EXPECT_EQ(calculateWastedSpace(group_of_two), 1000);

    std::vector<FileEntry> group_of_three = { {"file1", 1000}, {"file2", 1000}, {"file3", 1000} };
    EXPECT_EQ(calculateWastedSpace(group_of_three), 2000);
}

TEST(GuiFormattingTest, SortGroupsByWastedSpaceDescending) {
    std::vector<std::vector<FileEntry>> groups = {
        { {"a1", 100}, {"a2", 100} },                      // Wasted: 100
        { {"b1", 1000}, {"b2", 1000}, {"b3", 1000} },      // Wasted: 2000
        { {"c1", 500}, {"c2", 500} }                       // Wasted: 500
    };

    sortGroupsByWastedSpaceDescending(groups);

    ASSERT_EQ(groups.size(), 3);
    EXPECT_EQ(groups[0][0].path.string(), "b1");
    EXPECT_EQ(groups[1][0].path.string(), "c1");
    EXPECT_EQ(groups[2][0].path.string(), "a1");
}
