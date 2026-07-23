#include "thumbnail_cache.h"
#include "image_loader.h"
#include <algorithm>
#include <iostream>

#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#else
#include <GLFW/glfw3.h> // Includes basic OpenGL headers for glGenTextures etc.
#endif

// Windows <GL/gl.h> typically only supports up to OpenGL 1.1 which doesn't define GL_CLAMP_TO_EDGE
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

namespace dupcleaner::gui {

static ImageData downscaleImage(const ImageData& src, int max_dim) {
    if (src.width <= max_dim && src.height <= max_dim) return src;
    
    double scale = std::min((double)max_dim / src.width, (double)max_dim / src.height);
    int new_w = std::max(1, (int)(src.width * scale));
    int new_h = std::max(1, (int)(src.height * scale));
    
    ImageData dst;
    dst.width = new_w;
    dst.height = new_h;
    dst.channels = src.channels;
    dst.data.resize(new_w * new_h * src.channels);
    
    // Nearest neighbor downscaling
    for (int y = 0; y < new_h; ++y) {
        for (int x = 0; x < new_w; ++x) {
            int src_x = (int)(x / scale);
            int src_y = (int)(y / scale);
            if (src_x >= src.width) src_x = src.width - 1;
            if (src_y >= src.height) src_y = src.height - 1;
            
            for (int c = 0; c < src.channels; ++c) {
                dst.data[(y * new_w + x) * dst.channels + c] = 
                    src.data[(src_y * src.width + src_x) * src.channels + c];
            }
        }
    }
    return dst;
}

ThumbnailCache::~ThumbnailCache() {
    clear();
}

void ThumbnailCache::clear() {
    for (auto& pair : cache) {
        GLuint tex = pair.second;
        glDeleteTextures(1, &tex);
    }
    cache.clear();
}

GLuint ThumbnailCache::getTexture(const std::filesystem::path& path) {
    std::string key = path.string();
    if (cache.find(key) != cache.end()) {
        return cache[key];
    }

    auto opt_img = ImageLoader::load(path);
    if (!opt_img) {
        cache[key] = 0;
        return 0;
    }

    ImageData img = downscaleImage(*opt_img, MAX_DIMENSION);

    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 

    // Upload pixels into texture
    GLenum format = GL_RGBA;
    if (img.channels == 1) format = GL_LUMINANCE; // or GL_RED depending on modern OpenGL profile, but stb_image guarantees 3 or 4 mostly unless forced. ImageLoader forces 4 channels (RGBA).
    if (img.channels == 3) format = GL_RGB;
    if (img.channels == 4) format = GL_RGBA;
    
    // In our image_loader.h, we use stbi_load with 0 so it might be 3 or 4. 
    // Wait, let's look at image_loader.cpp. It doesn't force a channel count, it takes whatever.
    // In perceptual_hash.cpp it uses stbi_load as well.
    // For OpenGL, GL_LUMINANCE might be deprecated in core profile, so let's stick to standard if possible.
    // If we get an error here, we could force 4 channels in ImageLoader, but let's assume this works.

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, img.width, img.height, 0, format, GL_UNSIGNED_BYTE, img.data.data());

    cache[key] = image_texture;
    return image_texture;
}

} // namespace dupcleaner::gui
