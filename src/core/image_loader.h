#pragma once

#include <filesystem>
#include <vector>
#include <optional>

namespace dupcleaner {

struct ImageData {
    int width{0};
    int height{0};
    int channels{0};
    std::vector<unsigned char> data;
};

class ImageLoader {
public:
    // Loads an image from the given path.
    // Returns std::nullopt if the file is missing, not an image, or corrupt.
    static std::optional<ImageData> load(const std::filesystem::path& path);
};

} // namespace dupcleaner
