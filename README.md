# dupcleaner

**Status: early development**

dupcleaner scans a drive, finds exact and near-duplicate files/photos, and reports reclaimable space before deleting anything.

## Planned Features
- [ ] Scan directory trees
- [ ] Detect exact duplicates via hashing
- [ ] Detect near-duplicate photos (perceptual hashing)
- [ ] Report reclaimable space
- [ ] Safe deletion capabilities
- [ ] GUI frontend
- [ ] CLI frontend

## Dependencies

This project relies on the following libraries:

- **[stb_image.h](https://github.com/nothings/stb)**: Image loading for perceptual hashing (Vendored in `third_party/`).
- **[xxHash](https://github.com/Cyan4973/xxHash)**: Fast hashing for exact duplicate detection (CMake `FetchContent`, planned).
- **[GoogleTest](https://github.com/google/googletest)**: Unit testing framework (CMake `FetchContent`, planned).
- **[CLI11](https://github.com/CLIUtils/CLI11)**: Command-line parser (CMake `FetchContent`, planned).
- **[Dear ImGui](https://github.com/ocornut/imgui)**: Graphical user interface (CMake `FetchContent`, planned).
- **[GLFW](https://github.com/glfw/glfw)**: OpenGL window management for GUI (CMake `FetchContent`, planned).

## Build Instructions

```bash
cmake -S . -B build -DDUPCLEANER_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```
