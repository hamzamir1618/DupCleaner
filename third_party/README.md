# Vendored Third-Party Libraries

This directory contains single-header or drop-in libraries that are directly vendored into the repository. 
Other dependencies (like xxHash, GoogleTest, CLI11, Dear ImGui, GLFW) are fetched via CMake `FetchContent` during the build process to keep them version-pinned and easy to update.

## stb_image.h

- **Source URL**: https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
- **License**: Public Domain / MIT dual license
- **Commit Hash**: `013ac3beddff3dbffafd5177e7972067cd2b5083` (from [nothings/stb](https://github.com/nothings/stb))
- **Description**: Image loading library for perceptual hashing.

### 3. stb_image & stb_image_resize
- **Source**: [https://github.com/nothings/stb](https://github.com/nothings/stb)
- **License**: Public Domain (or MIT)
- **Version**: Current master as of July 2026
- **Purpose**: Single-header libraries for loading images (JPEG, PNG, BMP, etc.) from disk, and resizing them in-memory to generate extremely fast thumbnails without requiring heavy dependencies like OpenCV or ImageMagick.

### 4. tinyfiledialogs
- **Source**: [https://sourceforge.net/projects/tinyfiledialogs/](https://sourceforge.net/projects/tinyfiledialogs/)
- **License**: zlib
- **Version**: Current
- **Purpose**: Cross-platform C/C++ library for native open/save/folder selection dialogs. Used by the GUI to pick target directories without requiring large UI frameworks like Qt or GTK.

## stb_image_write.h

- **Source URL**: https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
- **License**: Public Domain / MIT dual license
- **Commit Hash**: `master` (from [nothings/stb](https://github.com/nothings/stb))
- **Description**: Image writing library for generating dynamic unit test payloads.

## Typography and Icons

### Roboto
- **Source URL**: https://github.com/google/fonts/tree/main/apache/roboto
- **License**: Apache 2.0
- **Description**: Standard clean UI font used for the interface.

### FontAwesome 6 & IconFontCppHeaders
- **Source URL**: https://github.com/FortAwesome/Font-Awesome and https://github.com/juliettef/IconFontCppHeaders
- **License**: FontAwesome Free License (SIL OFL 1.1 for fonts), MIT for Headers
- **Description**: Used to provide vector icons in the ImGui interface.
