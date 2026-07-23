# dupcleaner

[![CI](https://github.com/hamzamir1618/DupCleaner/actions/workflows/ci.yml/badge.svg)](https://github.com/hamzamir1618/DupCleaner/actions/workflows/ci.yml)

**Status: early development**

dupcleaner scans a drive, finds exact and near-duplicate files/photos, and reports reclaimable space before deleting anything.

## Current Capabilities
- [x] Scans directory trees recursively and reports file counts and scan statistics.
- [x] Detect exact duplicates via hashing and safe byte-for-byte verification.

## Planned Features
- [x] Detect near-duplicate photos (perceptual hashing)
- [ ] GUI frontend
- [x] Report reclaimable space
- [x] Safe deletion capabilities
- [x] CLI frontend

## Example Usage

### Finding Near-Duplicate Images
In addition to exact duplicates, you can scan for slightly altered images (e.g. resized, slightly cropped, compressed) using perceptual hashing:
```bash
$ dupcleaner_cli scan ./my_photos --include-near-duplicates --similarity-threshold 10
...
Found 1 near-duplicate image groups (Threshold: 10):

Near-Duplicate Group 1:
  - C:\my_photos\vacation.jpg (Reference Image)
  - C:\my_photos\edited\vacation_instagram.jpg (Similarity: 95% | Distance: 3)
```

### Human-Readable Output
```bash
$ dupcleaner_cli scan ./my_photos --min-size 1024
Found 1 exact duplicate groups:

Group 1 (Size: 1048576 bytes, Wasted: 1048576 bytes):
  - C:\my_photos\vacation.jpg
  - C:\my_photos\backup\vacation_copy.jpg

Total wasted space: 1048576 bytes.
```

### JSON Output
For programmatic integration, you can output the report as JSON:
```bash
$ dupcleaner_cli scan ./my_photos --json
{
  "groups": [
    {
      "files": [
        "C:\\my_photos\\vacation.jpg",
        "C:\\my_photos\\backup\\vacation_copy.jpg"
      ],
      "size_bytes": 1048576,
      "wasted_bytes": 1048576
    }
  ],
  "scan_stats": {
    "bytes_visited": 180524469,
    "directories_visited": 596,
    "duration_ms": 192,
    "files_visited": 1355,
    "items_skipped": 0
  },
  "total_wasted_bytes": 1048576
}
```

### Safely Cleaning Duplicates
To analyze and safely remove exact duplicates (keeping the oldest file in each group):
```bash
$ dupcleaner_cli clean ./my_photos --strategy oldest
Found 1 exact duplicate groups:
...
Total space to reclaim: 1048576 bytes

Proceed with deletion? [y/N]: y
Successfully moved files to trash.
```

By default, files are moved to a `.dupcleaner_trash/` directory inside your scanned folder. You can perform a dry-run first:
```bash
$ dupcleaner_cli clean ./my_photos --dry-run
```

### Interactive Review
For fine-grained control, you can review each duplicate group interactively to manually override suggestions or skip uncertain groups:
```bash
$ dupcleaner_cli clean ./my_photos --interactive
--- Exact Duplicate Group 1 of 1 ---
Reviewing Group:
  [1] C:\my_photos\vacation.jpg (1048576 bytes) (*Suggested*)
  [2] C:\my_photos\backup\vacation_copy.jpg (1048576 bytes)
Action (a=accept suggestion, s=skip group, k<N>=keep file N): k2

Deletion Plan:
...
```

> [!WARNING]  
> If you pass the `--permanent` flag, the files will be irreversibly unlinked from the filesystem and **cannot** be recovered! 

### Undoing Deletions
If you accidentally deleted files to the `.dupcleaner_trash/`, you can instantly restore the latest batch to their exact original pathways:
```bash
$ dupcleaner_cli undo ./my_photos
Successfully restored the most recent deletion batch.
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
