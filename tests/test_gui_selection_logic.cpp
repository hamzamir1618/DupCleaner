#include <gtest/gtest.h>
#include "selection_logic.h"

using namespace dupcleaner;
using namespace dupcleaner::gui;

TEST(GuiSelectionLogicTest, CalculateInitialSelections) {
    std::vector<FileEntry> group1 = {
        {"fileA.txt", 100},
        {"fileB.txt", 100}
    };
    std::vector<std::vector<FileEntry>> exact = {group1};

    DuplicateFinder::NearDuplicateResult near_dup;

    auto selected = calculateInitialSelections(exact, near_dup, KeepStrategy::KeepFirstAlphabetically);

    // KeepFirstAlphabetically should keep fileA.txt and delete fileB.txt
    EXPECT_EQ(selected.size(), 1);
    EXPECT_TRUE(selected.count("fileB.txt"));
    EXPECT_FALSE(selected.count("fileA.txt"));
}

TEST(GuiSelectionLogicTest, CalculateSelectedWastedSpace) {
    std::vector<std::vector<FileEntry>> exact = {
        { {"fileA.txt", 100}, {"fileB.txt", 100} }
    };
    
    DuplicateFinder::NearDuplicateGroup ng;
    ng.members = {
        { {"imgA.jpg", 200}, 0 },
        { {"imgB.jpg", 250}, 0 }
    };
    DuplicateFinder::NearDuplicateResult near_dup;
    near_dup.groups.push_back(ng);

    std::set<std::string> selected = {"fileB.txt", "imgB.jpg"};

    uintmax_t wasted = calculateSelectedWastedSpace(selected, exact, near_dup);
    EXPECT_EQ(wasted, 350);
}
