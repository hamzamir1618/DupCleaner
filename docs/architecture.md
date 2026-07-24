# Architecture

## Dependency graph

- **cli** -> `libdupcleaner` -> `xxHash`
- **gui** -> `libdupcleaner`
- **tests** -> `libdupcleaner` + `GoogleTest`

## Filesystem Traversal
The core logic utilizes a robust `DirectoryScanner` class wrapping `std::filesystem::recursive_directory_iterator`. It explicitly skips non-regular files and symlinks to prevent infinite directory recursion, and silently traps permission/read errors on a per-file basis using `std::error_code`. 
2. The `DirectoryScanner` recursively traverses paths, skipping hidden/system files and user-defined globs (`--exclude`). By skipping critical OS directories entirely using `it.disable_recursion_pending()`, the scanner avoids massive traversal latency and prevents users from inadvertently deleting necessary system duplicates (e.g., `WinSxS`).
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

Baseline performance is continuously measured against a synthetic test tree of 100,000 files spread across 10,000 directories (using the `gen_test_tree` utility) and validated against real-world full-drive scans.

### Synthetic Benchmark (100k files)
*   **Tree Generation**: ~1.2 seconds (SSD)
*   **Full Scan (Traversal + Hashing)**: ~150 - 250 milliseconds (SSD/Cached)
*   **Memory Footprint**: ~18 MB peak (during vector allocations)

### Real-World Full Drive Scan (D: Drive, 400k+ items)
*   **Environment**: Spinning Disk / Standard VM, ~416,000 files, ~45,000 directories.
*   **Traversal Phase**: ~30.2 seconds (Massive speedup due to `directory_entry` caching and `--exclude` system skipping).
*   **Size-Bucketing**: ~0.5 seconds (Filtering 416k down to 366k candidates).
*   **Fingerprinting**: ~10 minutes (IO-bound limit at ~0.4 MB/s randomly seeking across 366k candidate files. Concurrency tuned to `min(hardware_concurrency, 4)` to prevent disk thrashing).
*   **Takeaway**: The directory traversal is no longer a bottleneck. Future optimizations should focus exclusively on intelligent byte-reading strategies or skipping hashing for known files.

The architecture emphasizes doing the absolute minimum amount of work necessary. The F5.1 F5.3 F5.5 enhancements ensure that even full-drive scans complete traversal smoothly, gracefully prune OS system directories, and keep memory usage strictly bounded, while the IO-bound hashing phase scales safely.

Because this test generates several gigabytes of data and takes significant time, it is excluded from the default CI suite. To run it explicitly:
```bash
cmake --build build --target perf_scan_benchmark
ctest --test-dir build -C Release -L performance -V
```
*Note: A baseline run on a modern NVMe SSD (100,000 files, up to 64KB each) should yield a scan throughput upwards of 10,000 files/sec.*

### GUI Threading Model (ScanController)
The GUI (`dupcleaner_gui`) is intentionally thin, focusing purely on presentation and delegating work to the Core library.

- **Scan Lifecycle and Ownership**: The GUI relies on a single `ScanController` instance (owned by `DupCleanerApp`) to manage the background scanning thread. The raw `std::thread` is never exposed to the UI layer, preventing it from being destructed while running (which would cause a silent `std::terminate`).
- **Synchronization**: `ScanController` safely communicates state (Idle, Scanning, Done, Failed) via `std::atomic` enums. It encapsulates a `std::mutex` to protect results and error messages. The main ImGui render loop queries these thread-safe accessors every frame to update the UI without blocking.
- **Cancellation**: `ScanController` supports early cancellation by passing an `std::atomic<bool>*` to `DirectoryScanner`, which checks it periodically during filesystem traversal and aborts cleanly.
- **Exception Boundary**: The background scan lambda wraps all Core invocations in a `try/catch` block. Any exceptions are trapped, serialized to a string, and exposed via the `Failed` state, guaranteeing the thread never leaks exceptions to the OS.

- **Thumbnail Generation and Caching**: To help users visually distinguish exact and near-duplicates, the GUI displays thumbnails. 
  - Thumbnails are loaded lazily on the main render thread via `ImageLoader`. 
  - To minimize memory consumption, images are aggressively downscaled on the CPU (using a nearest-neighbor algorithm to a max dimension of 256px) *before* being uploaded as an OpenGL texture (`glTexImage2D`).
  - Textures are cached in a `ThumbnailCache` (`std::unordered_map<std::string, GLuint>`) keyed by the image's absolute path. This prevents re-decoding and re-uploading the same image every frame. The cache clears OpenGL textures gracefully upon destruction.

## Testing Strategy
The core library logic is rigorously unit-tested via GoogleTest (e.g. hashing, file size grouping, path traversal, deletion tracking). The GUI frontend, however, cannot be meaningfully unit-tested or run headless in standard CI pipelines. As a result, the GUI app is deliberately kept as thin as possible, serving only as a visual shell that delegates all heavy lifting to `libdupcleaner`. GUI testing is performed via manual verification, while the CI pipeline exclusively verifies that the GUI target compiles successfully without errors across all platforms.
