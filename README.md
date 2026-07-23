# dupcleaner

[![CI](https://github.com/hamzamir1618/DupCleaner/actions/workflows/ci.yml/badge.svg)](https://github.com/hamzamir1618/DupCleaner/actions/workflows/ci.yml)

**Status: early development**

dupcleaner scans a drive, finds exact and near-duplicate files/photos, and reports reclaimable space before deleting anything.

## Current Capabilities
- [x] Scans directory trees recursively and reports file counts and scan statistics.
- [x] Detect exact duplicates via hashing and safe byte-for-byte verification.

## Planned Features
- [ ] Detect near-duplicate photos (perceptual hashing)
- [ ] Report reclaimable space
- [ ] Safe deletion capabilities
- [ ] GUI frontend
- [ ] CLI frontend

## Example Usage
```bash
$ dupcleaner_cli --path ./my_photos
Scanning directory: C:\my_photos...

--- Scan Complete ---
Total files visited: 1355
Total bytes visited: 180524469 bytes

Finding exact duplicates...

Found 1 exact duplicate groups:

Group 1 (Size: 1048576 bytes, Wasted: 1048576 bytes):
  - C:\my_photos\vacation.jpg
  - C:\my_photos\backup\vacation_copy.jpg

Total wasted space: 1048576 bytes.
```
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

## Contributing

Please read our [Contributing Guidelines](CONTRIBUTING.md) before submitting Pull Requests to ensure your tests, documentation, and CI workflows meet project conventions.
