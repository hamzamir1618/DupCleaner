#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <chrono>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "image_loader.h"

namespace fs = std::filesystem;
using namespace dupcleaner;

class ImageLoaderTest : public ::testing::Test {
protected:
    fs::path temp_dir;

    void SetUp() override {
        auto now = std::chrono::system_clock::now().time_since_epoch().count();
        temp_dir = fs::temp_directory_path() / ("dupcleaner_img_test_" + std::to_string(now));
        fs::create_directories(temp_dir);
    }

    void TearDown() override {
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
    }

    fs::path create_text_file(const std::string& name) {
        fs::path p = temp_dir / name;
        std::ofstream ofs(p);
        ofs << "Not an image file at all!";
        ofs.close();
        return p;
    }

    fs::path create_corrupt_png(const std::string& name) {
        fs::path p = temp_dir / name;
        std::ofstream ofs(p, std::ios::binary);
        // Write PNG header but nothing else
        const unsigned char png_sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
        ofs.write(reinterpret_cast<const char*>(png_sig), sizeof(png_sig));
        ofs.close();
        return p;
    }

    fs::path create_valid_png(const std::string& name, int width, int height) {
        fs::path p = temp_dir / name;
        std::vector<unsigned char> pixels(width * height * 3, 255); // White image
        stbi_write_png(p.string().c_str(), width, height, 3, pixels.data(), width * 3);
        return p;
    }
};

TEST_F(ImageLoaderTest, ValidImageLoadsCorrectly) {
    auto p = create_valid_png("valid.png", 10, 15);
    auto img = ImageLoader::load(p);

    ASSERT_TRUE(img.has_value());
    EXPECT_EQ(img->width, 10);
    EXPECT_EQ(img->height, 15);
    EXPECT_EQ(img->channels, 3);
    EXPECT_EQ(img->data.size(), 10 * 15 * 3);
}

TEST_F(ImageLoaderTest, NonImageFileReturnsNullopt) {
    auto p = create_text_file("text.png");
    auto img = ImageLoader::load(p);
    EXPECT_FALSE(img.has_value());
}

TEST_F(ImageLoaderTest, CorruptImageReturnsNullopt) {
    auto p = create_corrupt_png("corrupt.png");
    auto img = ImageLoader::load(p);
    EXPECT_FALSE(img.has_value());
}

TEST_F(ImageLoaderTest, MissingFileReturnsNullopt) {
    auto img = ImageLoader::load(temp_dir / "does_not_exist.png");
    EXPECT_FALSE(img.has_value());
}
