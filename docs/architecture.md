# Architecture

## Dependency graph

- **cli** -> `libdupcleaner` -> `xxHash`
- **gui** -> `libdupcleaner`
- **tests** -> `libdupcleaner` + `GoogleTest`

## Filesystem Traversal
The core logic utilizes a robust `DirectoryScanner` class wrapping `std::filesystem::recursive_directory_iterator`. It explicitly skips non-regular files and symlinks to prevent infinite directory recursion, and silently traps permission/read errors on a per-file basis using `std::error_code`. 2. The `DirectoryScanner` recursively traverses paths, skipping hidden/system files, and collects `FileEntry` structs (path and size).
3. The `DuplicateFinder` uses a lightweight, custom `ThreadPool` to dispatch work. Files are grouped by exact size into buckets. Each bucket that contains more than one file is processed by a worker thread.
4. The worker thread hashes files in its bucket concurrently using the `FileHasher` (which relies on `xxHash` for speed).
5. Files with matching hashes inside each bucket are then compared byte-for-byte in chunks to guarantee zero false positives.
6. The exact group vectors are then sorted deterministically before being returned. Instead of aborting an entire scan of 100k+ files when encountering a single inaccessible file, it collects warnings in a skipped-paths ledger (`getSkippedPaths()`) ensuring large real-world scans finish successfully.

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

## Performance Testing
To ensure the application scales gracefully to massive photo libraries, a synthetic data generator (`gen_test_tree`) and a performance benchmark test (`perf_scan_benchmark`) are included.

The benchmark generates a configurable synthetic directory tree (default 100,000 files, 20% duplicate ratio) and times both the filesystem traversal (`DirectoryScanner`) and the hashing/comparison pass (`DuplicateFinder::findExactDuplicates`). 

Because this test generates several gigabytes of data and takes significant time, it is excluded from the default CI suite. To run it explicitly:
```bash
cmake --build build --target perf_scan_benchmark
ctest --test-dir build -C Release -L performance -V
```
*Note: A baseline run on a modern NVMe SSD (100,000 files, up to 64KB each) should yield a scan throughput upwards of 10,000 files/sec.*

### GUI Logic and Render Loop
The GUI (`dupcleaner_gui`) is intentionally thin, focusing purely on presentation and delegating work to the Core library.

- **Background Scanning**: The GUI invokes `DirectoryScanner` and `DuplicateFinder` on a separate `std::thread`. A `std::atomic<bool>` flag and a `std::mutex` safely communicate completion and transfer results back to the ImGui render loop, ensuring the UI remains perfectly responsive during long I/O operations.

- **Thumbnail Generation and Caching**: To help users visually distinguish exact and near-duplicates, the GUI displays thumbnails. 
  - Thumbnails are loaded lazily on the main render thread via `ImageLoader`. 
  - To minimize memory consumption, images are aggressively downscaled on the CPU (using a nearest-neighbor algorithm to a max dimension of 256px) *before* being uploaded as an OpenGL texture (`glTexImage2D`).
  - Textures are cached in a `ThumbnailCache` (`std::unordered_map<std::string, GLuint>`) keyed by the image's absolute path. This prevents re-decoding and re-uploading the same image every frame. The cache clears OpenGL textures gracefully upon destruction.

## Testing Strategy
The core library logic is rigorously unit-tested via GoogleTest (e.g. hashing, file size grouping, path traversal, deletion tracking). The GUI frontend, however, cannot be meaningfully unit-tested or run headless in standard CI pipelines. As a result, the GUI app is deliberately kept as thin as possible, serving only as a visual shell that delegates all heavy lifting to `libdupcleaner`. GUI testing is performed via manual verification, while the CI pipeline exclusively verifies that the GUI target compiles successfully without errors across all platforms.
