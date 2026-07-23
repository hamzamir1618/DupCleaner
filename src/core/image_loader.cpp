#include "image_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace dupcleaner {

std::optional<ImageData> ImageLoader::load(const std::filesystem::path& path) {
    int w, h, c;
    
    // Attempt to load the image.
    // If the file is not an image or fails to load, stbi_load returns null.
    unsigned char* raw_data = stbi_load(path.string().c_str(), &w, &h, &c, 0);

    if (!raw_data) {
        return std::nullopt;
    }

    ImageData img;
    img.width = w;
    img.height = h;
    img.channels = c;
    
    // Copy the raw bytes into our vector so we can safely free the stb memory allocation
    size_t data_size = static_cast<size_t>(w) * static_cast<size_t>(h) * static_cast<size_t>(c);
    img.data.assign(raw_data, raw_data + data_size);

    stbi_image_free(raw_data);

    return img;
}

} // namespace dupcleaner
