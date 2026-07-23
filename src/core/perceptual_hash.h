#pragma once
#include <cstdint>
#include "image_loader.h"

namespace dupcleaner {

class PerceptualHash {
public:
    // Computes a 64-bit dHash (difference hash) for the given image.
    // The image is internally resized to 9x8, grayscaled, and adjacent pixels are compared.
    static uint64_t computeDHash(const ImageData& img);

    // Computes the Hamming distance between two 64-bit hashes.
    // Represents the number of differing bits.
    static int hammingDistance(uint64_t a, uint64_t b);
};

} // namespace dupcleaner
