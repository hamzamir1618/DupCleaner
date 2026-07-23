#include "perceptual_hash.h"

namespace dupcleaner {

uint64_t PerceptualHash::computeDHash(const ImageData& img) {
    if (img.width == 0 || img.height == 0 || img.data.empty() || img.channels == 0) {
        return 0;
    }

    int gray_9x8[8][9];

    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 9; ++x) {
            int start_x = x * img.width / 9;
            int end_x = (x + 1) * img.width / 9;
            int start_y = y * img.height / 8;
            int end_y = (y + 1) * img.height / 8;
            
            // Handle edge cases where image is smaller than 9x8
            if (end_x <= start_x) end_x = start_x + 1;
            if (end_y <= start_y) end_y = start_y + 1;
            
            if (end_x > img.width) end_x = img.width;
            if (end_y > img.height) end_y = img.height;

            uint64_t sum_luma = 0;
            int count = 0;
            
            for (int sy = start_y; sy < end_y; ++sy) {
                for (int sx = start_x; sx < end_x; ++sx) {
                    int idx = (sy * img.width + sx) * img.channels;
                    unsigned char r = img.data[idx];
                    unsigned char g = (img.channels >= 3) ? img.data[idx + 1] : r;
                    unsigned char b = (img.channels >= 3) ? img.data[idx + 2] : r;
                    
                    // CCIR 601 luma formula
                    sum_luma += (r * 299 + g * 587 + b * 114) / 1000;
                    count++;
                }
            }
            
            gray_9x8[y][x] = count > 0 ? static_cast<int>(sum_luma / count) : 0;
        }
    }

    uint64_t hash = 0;
    int bit_index = 0;
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            // Set bit to 1 if left pixel is brighter than right pixel
            // Some implementations do strictly < or >, we'll use < for left-to-right gradient positive
            if (gray_9x8[y][x] < gray_9x8[y][x + 1]) {
                hash |= (1ULL << bit_index);
            }
            bit_index++;
        }
    }

    return hash;
}

int PerceptualHash::hammingDistance(uint64_t a, uint64_t b) {
    uint64_t diff = a ^ b;
    int dist = 0;
    while (diff) {
        diff &= (diff - 1);
        dist++;
    }
    return dist;
}

} // namespace dupcleaner
