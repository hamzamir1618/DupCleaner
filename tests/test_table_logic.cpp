#include <gtest/gtest.h>
#include "table_logic.h"

using namespace dupcleaner;
using namespace dupcleaner::gui;

TEST(TableLogicTest, SortsByPath) {
    std::vector<FileEntry> entries;
    entries.push_back({std::filesystem::path("C:/z.txt"), 100, {}});
    entries.push_back({std::filesystem::path("C:/a.txt"), 200, {}});
    entries.push_back({std::filesystem::path("C:/m.txt"), 150, {}});

    SortFileEntries(entries, TableColumn::Path, false);
    EXPECT_EQ(entries[0].path.string(), "C:/a.txt");
    EXPECT_EQ(entries[1].path.string(), "C:/m.txt");
    EXPECT_EQ(entries[2].path.string(), "C:/z.txt");

    SortFileEntries(entries, TableColumn::Path, true);
    EXPECT_EQ(entries[0].path.string(), "C:/z.txt");
    EXPECT_EQ(entries[1].path.string(), "C:/m.txt");
    EXPECT_EQ(entries[2].path.string(), "C:/a.txt");
}

TEST(TableLogicTest, SortsBySizeAndFallback) {
    std::vector<FileEntry> entries;
    entries.push_back({std::filesystem::path("C:/b.txt"), 100, {}});
    entries.push_back({std::filesystem::path("C:/c.txt"), 200, {}});
    entries.push_back({std::filesystem::path("C:/a.txt"), 100, {}}); // Same size as b.txt

    SortFileEntries(entries, TableColumn::Size, false);
    EXPECT_EQ(entries[0].path.string(), "C:/a.txt");
    EXPECT_EQ(entries[1].path.string(), "C:/b.txt");
    EXPECT_EQ(entries[2].path.string(), "C:/c.txt");
}
