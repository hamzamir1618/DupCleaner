#pragma once

#include <filesystem>
#include <unordered_map>
#include <string>

// Forward declaration of GLuint to avoid including OpenGL headers in the header
typedef unsigned int GLuint;

namespace dupcleaner::gui {

class ThumbnailCache {
public:
    ThumbnailCache() = default;
    ~ThumbnailCache();

    // Returns a valid OpenGL texture ID for the given path, or 0 if it fails to load.
    // Automatically downscales the image on CPU to save GPU memory and caches it.
    GLuint getTexture(const std::filesystem::path& path);

    // Clears all cached textures from GPU memory.
    void clear();

private:
    std::unordered_map<std::string, GLuint> cache;
    static constexpr int MAX_DIMENSION = 256;
};

} // namespace dupcleaner::gui
