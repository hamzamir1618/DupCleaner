#include <gtest/gtest.h>
#include "perceptual_hash.h"
#include <vector>

using namespace dupcleaner;

class PerceptualHashTest : public ::testing::Test {
protected:
    ImageData create_synthetic_image(int width, int height, bool gradient, bool inverted = false) {
        ImageData img;
        img.width = width;
        img.height = height;
        img.channels = 3;
        img.data.resize(static_cast<size_t>(width) * height * 3);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * 3;
                unsigned char val = 0;
                if (gradient) {
                    val = (x * 255) / width;
                } else {
                    // Checkerboard
                    val = ((x / 10) % 2 == (y / 10) % 2) ? 255 : 0;
                }
                
                if (inverted) val = 255 - val;
                
                img.data[idx] = val;
                img.data[idx+1] = val;
                img.data[idx+2] = val;
            }
        }
        return img;
    }
};

TEST_F(PerceptualHashTest, IdenticalImagesProduceIdenticalHashes) {
    auto img1 = create_synthetic_image(100, 100, true);
    auto img2 = create_synthetic_image(100, 100, true);
    
    uint64_t hash1 = PerceptualHash::computeDHash(img1);
    uint64_t hash2 = PerceptualHash::computeDHash(img2);
    
    EXPECT_EQ(hash1, hash2);
}

TEST_F(PerceptualHashTest, DifferentDimensionsProduceIdenticalHashesForSameGradient) {
    auto img1 = create_synthetic_image(100, 100, true);
    auto img2 = create_synthetic_image(200, 150, true);
    
    uint64_t hash1 = PerceptualHash::computeDHash(img1);
    uint64_t hash2 = PerceptualHash::computeDHash(img2);
    
    EXPECT_EQ(hash1, hash2);
}

TEST_F(PerceptualHashTest, AlteredPixelProducesSmallHammingDistance) {
    auto img1 = create_synthetic_image(100, 100, true);
    auto img2 = create_synthetic_image(100, 100, true);
    
    // Alter a few pixels to simulate small visual artifact or noise
    for(int i = 0; i < 30; ++i) {
        img2.data[i] = 255 - img2.data[i];
    }
    
    uint64_t hash1 = PerceptualHash::computeDHash(img1);
    uint64_t hash2 = PerceptualHash::computeDHash(img2);
    
    int dist = PerceptualHash::hammingDistance(hash1, hash2);
    // Should be very small, usually 0 or 1 because the box filter absorbs small noise
    EXPECT_LT(dist, 5); 
}

TEST_F(PerceptualHashTest, DifferentImagesProduceLargeHammingDistance) {
    auto img1 = create_synthetic_image(100, 100, true, false);
    auto img2 = create_synthetic_image(100, 100, true, true); // Inverted gradient
    
    uint64_t hash1 = PerceptualHash::computeDHash(img1);
    uint64_t hash2 = PerceptualHash::computeDHash(img2);
    
    int dist = PerceptualHash::hammingDistance(hash1, hash2);
    EXPECT_GT(dist, 20); // Should be significantly different
}

TEST_F(PerceptualHashTest, StandaloneHammingHelper) {
    EXPECT_EQ(PerceptualHash::hammingDistance(0b0000ULL, 0b0000ULL), 0);
    EXPECT_EQ(PerceptualHash::hammingDistance(0b1010ULL, 0b0101ULL), 4);
    EXPECT_EQ(PerceptualHash::hammingDistance(0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL), 64);
}
