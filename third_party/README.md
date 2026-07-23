# Vendored Third-Party Libraries

This directory contains single-header or drop-in libraries that are directly vendored into the repository. 
Other dependencies (like xxHash, GoogleTest, CLI11, Dear ImGui, GLFW) are fetched via CMake `FetchContent` during the build process to keep them version-pinned and easy to update.

## stb_image.h

- **Source URL**: https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
- **License**: Public Domain / MIT dual license
- **Commit Hash**: `013ac3beddff3dbffafd5177e7972067cd2b5083` (from [nothings/stb](https://github.com/nothings/stb))
- **Description**: Image loading library for perceptual hashing.

## stb_image_write.h

- **Source URL**: https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
- **License**: Public Domain / MIT dual license
- **Commit Hash**: `master` (from [nothings/stb](https://github.com/nothings/stb))
- **Description**: Image writing library for generating dynamic unit test payloads.
