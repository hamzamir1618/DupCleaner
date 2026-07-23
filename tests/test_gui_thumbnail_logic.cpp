#include <gtest/gtest.h>
#include "thumbnail_logic.h"

using namespace dupcleaner;
using namespace dupcleaner::gui;

TEST(GuiThumbnailLogicTest, IsImageGroup) {
    EXPECT_FALSE(isImageGroup({}));

    std::vector<FileEntry> text_group = { {"file.txt", 10}, {"file2.txt", 10} };
    EXPECT_FALSE(isImageGroup(text_group));

    std::vector<FileEntry> img_group1 = { {"photo.jpg", 100} };
    EXPECT_TRUE(isImageGroup(img_group1));

    std::vector<FileEntry> img_group2 = { {"photo.PNG", 100} };
    EXPECT_TRUE(isImageGroup(img_group2));

    std::vector<FileEntry> img_group3 = { {"photo.jpeg", 100} };
    EXPECT_TRUE(isImageGroup(img_group3));
}

TEST(GuiThumbnailLogicTest, GetPathsForThumbnails) {
    std::vector<FileEntry> group = {
        {"img1.jpg", 100},
        {"img2.jpg", 100},
        {"img3.jpg", 100}
    };

    // Exact duplicates: only need 1 thumbnail
    auto exact_paths = getPathsForThumbnails(group, false);
    ASSERT_EQ(exact_paths.size(), 1);
    EXPECT_EQ(exact_paths[0].string(), "img1.jpg");

    // Near duplicates: need all thumbnails
    auto near_paths = getPathsForThumbnails(group, true);
    ASSERT_EQ(near_paths.size(), 3);
    EXPECT_EQ(near_paths[0].string(), "img1.jpg");
    EXPECT_EQ(near_paths[1].string(), "img2.jpg");
    EXPECT_EQ(near_paths[2].string(), "img3.jpg");
}
