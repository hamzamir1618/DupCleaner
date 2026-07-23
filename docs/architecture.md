# Architecture

## Dependency graph

- **cli** -> `libdupcleaner` -> `xxHash`
- **gui** -> `libdupcleaner`
- **tests** -> `libdupcleaner` + `GoogleTest`

## Filesystem Traversal
The core logic utilizes a robust `DirectoryScanner` class wrapping `std::filesystem::recursive_directory_iterator`. It explicitly skips non-regular files and symlinks to prevent infinite directory recursion, and silently traps permission/read errors on a per-file basis using `std::error_code`. Instead of aborting an entire scan of 100k+ files when encountering a single inaccessible file, it collects warnings in a skipped-paths ledger (`getSkippedPaths()`) ensuring large real-world scans finish successfully.

## Fast Hashing (Fingerprinting)
To quickly sort potential duplicates before performing expensive byte-for-byte comparisons, `dupcleaner` uses `xxHash64` (`FileHasher`). xxHash was chosen specifically for its extreme speed on modern CPUs (often bottlenecking on disk I/O rather than CPU). It is a non-cryptographic hash, which is perfectly fine for this use case since we use it merely as a fast fingerprinting heuristic, and any collisions will be securely verified with a full byte-for-byte comparison before any deletion operations occur.

## Safety Model
When executing deletion operations, `dupcleaner` defaults to moving duplicate files into a localized, hidden `.dupcleaner_trash/` directory situated within the scanned target folder. Along with the files, it generates a JSON manifest (`manifest.json`) that maps the randomized trash filenames back to their exact original absolute paths. This default "trash" behavior guarantees that users can completely reverse accidental deletions via the `undo` command. Permanent deletion (`std::filesystem::remove`) is strictly an opt-in behavior (`--permanent`) and avoids touching any user system-level trash configurations, keeping operations strictly sandboxed to the project directory being analyzed.

## Perceptual Hashing (dHash)
To identify near-duplicate images (e.g. resized, slightly cropped, or compressed variants of the same photo), `dupcleaner` implements the **dHash** (difference hash) algorithm.
dHash was chosen for Phase 2 over algorithms like pHash (DCT-based) or aHash (average hash) because it offers an excellent tradeoff: it is highly resistant to resizing and brightness/contrast adjustments, while remaining extremely fast to compute and simple to implement from scratch. 
1. The image is downscaled to a tiny `9x8` grid using a localized box-filter to accurately capture regional luminosity.
2. The grid is converted to grayscale using the CCIR 601 luma formula.
3. Each row is scanned pixel by pixel: if the left pixel is brighter than the right pixel, a bit is flipped. This yields exactly 64 bits of gradient data, encoded as a `uint64_t`.
4. Image similarity is calculated by checking the Hamming distance (`std::popcount(a ^ b)`) between two hashes.

*Note: In the future, a DCT-based pHash may be integrated for higher accuracy against extreme crops or watermarks.*

### GUI Logic and Render Loop
The GUI (`dupcleaner_gui`) is intentionally thin, focusing purely on presentation and delegating work to the Core library.

- **Background Scanning**: The GUI invokes `DirectoryScanner` and `DuplicateFinder` on a separate `std::thread`. A `std::atomic<bool>` flag and a `std::mutex` safely communicate completion and transfer results back to the ImGui render loop, ensuring the UI remains perfectly responsive during long I/O operations.

- **Thumbnail Generation and Caching**: To help users visually distinguish exact and near-duplicates, the GUI displays thumbnails. 
  - Thumbnails are loaded lazily on the main render thread via `ImageLoader`. 
  - To minimize memory consumption, images are aggressively downscaled on the CPU (using a nearest-neighbor algorithm to a max dimension of 256px) *before* being uploaded as an OpenGL texture (`glTexImage2D`).
  - Textures are cached in a `ThumbnailCache` (`std::unordered_map<std::string, GLuint>`) keyed by the image's absolute path. This prevents re-decoding and re-uploading the same image every frame. The cache clears OpenGL textures gracefully upon destruction.

## Testing Strategy
The core library logic is rigorously unit-tested via GoogleTest (e.g. hashing, file size grouping, path traversal, deletion tracking). The GUI frontend, however, cannot be meaningfully unit-tested or run headless in standard CI pipelines. As a result, the GUI app is deliberately kept as thin as possible, serving only as a visual shell that delegates all heavy lifting to `libdupcleaner`. GUI testing is performed via manual verification, while the CI pipeline exclusively verifies that the GUI target compiles successfully without errors across all platforms.
